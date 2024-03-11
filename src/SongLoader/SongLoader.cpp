#include "SongLoader/SongLoader.hpp"
#include "CustomJSONData.hpp"
#include "SongLoader/SongCoreCustomLevelPack.hpp"
#include "main.hpp"
#include "shared/SongCore.hpp"
#include "logging.hpp"
#include "config.hpp"
#include "assets.hpp"
#include "paper/shared/utfcpp/source/utf8.h"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"

#include "Utils/Hashing.hpp"
#include "Utils/File.hpp"
#include "GlobalNamespace/PlayerSaveData.hpp"
#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/PreviewDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapLevelColorSchemeSaveData.hpp"
#include "GlobalNamespace/EnvironmentsListSO.hpp"
#include "System/Collections/Generic/ICollection_1.hpp"
#include "System/Collections/Generic/IEnumerable_1.hpp"
#include "System/Collections/Generic/IEnumerator_1.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "System/Collections/Generic/HashSet_1.hpp"
#include "System/IDisposable.hpp"

#include "bsml/shared/Helpers/utilities.hpp"

#include <chrono>
#include <clocale>
#include <exception>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <ostream>
#include <ratio>
#include <stdexcept>
#include <string.h>
#include <string>

DEFINE_TYPE(SongCore::SongLoader, RuntimeSongLoader);

#define MAX_THREAD_COUNT 8

using namespace std::chrono;

namespace SongCore::SongLoader {
    void RuntimeSongLoader::ctor() {
        INVOKE_BASE_CTOR(classof(UnityEngine::MonoBehaviour*));
        INVOKE_CTOR();

        _customLevelPack = SongCoreCustomLevelPack::New(fmt::format("{}CustomLevels", CUSTOM_LEVEL_PACK_PREFIX_ID), "Custom Levels", nullptr);
        _customWIPLevelPack = SongCoreCustomLevelPack::New(fmt::format("{}CustomWIPLevels", CUSTOM_LEVEL_PACK_PREFIX_ID), "Custom WIP Levels", nullptr);
        _customLevelPackCollection = SongCoreCustomBeatmapLevelPackCollection::New();

        _customLevels = SongDict::New_ctor();
        _customWIPLevels = SongDict::New_ctor();
    }

    void RuntimeSongLoader::Awake() {
        auto customLevelPackID = _customLevelPack->get_packID();
        auto customWIPLevelPackID = _customWIPLevelPack->get_packID();

        auto alwaysOwnedPackIds = _additionalContentModel->_alwaysOwnedPacksIds;
        if (!alwaysOwnedPackIds->Contains(customLevelPackID)) alwaysOwnedPackIds->Add(customLevelPackID);
        if (!alwaysOwnedPackIds->Contains(customWIPLevelPackID)) alwaysOwnedPackIds->Add(customWIPLevelPackID);

        // set default pack covers on custom levels
        _customLevelPack->_coverImage_k__BackingField = _customLevelLoader->_defaultPackCover;
        _customLevelPack->_smallCoverImage_k__BackingField = _customLevelLoader->_smallDefaultPackCover;

        // set custom pack cover for regular levels
        _customWIPLevelPack->_coverImage_k__BackingField = BSML::Utilities::LoadSpriteRaw(Assets::Resources::CustomWIPLevelsCover_png);
        _customWIPLevelPack->_smallCoverImage_k__BackingField = _customWIPLevelPack->_coverImage_k__BackingField;

        RefreshSongs(true);
    }

    void RuntimeSongLoader::Inject(GlobalNamespace::CustomLevelLoader* customLevelLoader, GlobalNamespace::BeatmapLevelsModel* beatmapLevelsModel, GlobalNamespace::CachedMediaAsyncLoader* cachedMediaAsyncLoader, GlobalNamespace::BeatmapCharacteristicCollection* beatmapCharacteristicCollection, GlobalNamespace::IAdditionalContentModel* additionalContentModel) {
        _beatmapLevelsModel = beatmapLevelsModel;
        _customLevelLoader = customLevelLoader;
        _cachedMediaAsyncLoader = cachedMediaAsyncLoader;
        _beatmapCharacteristicCollection = beatmapCharacteristicCollection;
        _additionalContentModel = il2cpp_utils::cast<GlobalNamespace::AdditionalContentModel>(additionalContentModel);
    }

    void RuntimeSongLoader::CollectLevels(std::span<const std::filesystem::path> roots, bool isWip, std::set<LevelPathAndWip>& out) {
        for (auto& rootPath : roots) {
            if (!std::filesystem::exists(rootPath)) {
                WARNING("Attempted to load songs from folder '{}' but it did not exist! skipping...", rootPath.string());
                continue;
            }

            auto songPaths = Utils::GetFolders(rootPath);
            for (auto& songPath : songPaths) {
                auto dataPath = songPath / "info.dat";
                if (!std::filesystem::exists(dataPath)) dataPath = songPath / "Info.dat";
                if (!std::filesystem::exists(dataPath)) {
                    WARNING("Song path '{}' had no info.dat! skipping...", songPath.string());
                    continue;
                }

                out.emplace(songPath, isWip);
            }
        }
    }

    std::shared_future<void> RuntimeSongLoader::RefreshSongs(bool fullRefresh) {
        if (AreSongsRefreshing) {
            return _currentlyLoadingFuture;
        }

        SongsWillRefresh.invoke();
        _areSongsLoaded = false;

        _currentlyLoadingFuture = il2cpp_utils::il2cpp_async(std::launch::async, &RuntimeSongLoader::RefreshSongs_internal, this, std::forward<bool>(fullRefresh));
        return _currentlyLoadingFuture;
    }

    void RuntimeSongLoader::RefreshSongs_internal(bool fullRefresh) {
        std::set<LevelPathAndWip> levels;

        // travel the given song paths to collect levels to load
        CollectLevels(config.RootCustomLevelPaths, false, levels);
        CollectLevels(config.RootCustomWIPLevelPaths, false, levels);


        // load songs on multiple threads
        std::mutex levelsItrMutex;
        std::set<LevelPathAndWip>::const_iterator levelsItr = levels.begin();
        std::set<LevelPathAndWip>::const_iterator levelsEnd = levels.end();

        using namespace std::chrono;
        auto startTime = high_resolution_clock::now();

        std::vector<il2cpp_utils::il2cpp_aware_thread> workerThreads;
        workerThreads.reserve(MAX_THREAD_COUNT);

        _totalSongs = levels.size();
        _loadProgress = 0;

        for (int i = 0; i < MAX_THREAD_COUNT; i++) {
            workerThreads.emplace_back(
                &RuntimeSongLoader::RefreshSongWorkerThread,
                this,
                &levelsItrMutex,
                &levelsItr,
                &levelsEnd
            );
        }

        for (auto& t : workerThreads) {
            t.join();
        }

        size_t actualCount = _customLevels->Count + _customWIPLevels->Count;
        INFO("Loaded {} (actual: {}) songs in {}ms", (size_t)_totalSongs, actualCount, duration_cast<milliseconds>(high_resolution_clock::now() - startTime).count());
        _loadProgress = 0.0f;

        // anonymous function to get the values from a songdict into a vector
        static auto GetValues = [](SongDict* dict){
            std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> vec;
            vec.reserve(dict->Count);

            auto enumerator = dict->GetEnumerator();
            while(enumerator->i___System__Collections__IEnumerator()->MoveNext()) {
                vec.emplace_back(enumerator->Current.Value);
            }
            enumerator->i___System__IDisposable()->Dispose();

            return vec;
        };

        auto customLevelValues = GetValues(_customLevels);
        auto customWIPLevelValues = GetValues(_customWIPLevels);

        _customLevelPack->SetLevels(customLevelValues);
        _customLevelPack->SortLevels();

        _customWIPLevelPack->SetLevels(customWIPLevelValues);
        _customWIPLevelPack->SortLevels();

        _allLoadedLevels.clear();
        _allLoadedLevels.reserve(actualCount);

        // insert wip levels before other loaded levels
        _allLoadedLevels.insert(_allLoadedLevels.begin(), customWIPLevelValues.begin(), customWIPLevelValues.end());
        _allLoadedLevels.insert(_allLoadedLevels.begin(), customLevelValues.begin(), customLevelValues.end());

        _areSongsLoaded = true;

        auto _allLevelIds = _additionalContentModel->_alwaysOwnedBeatmapLevelIds;
        for (auto level : _allLoadedLevels) {
            auto levelID = level->levelID;
            if (!_allLevelIds->Contains(levelID))
                _allLevelIds->Add(levelID);
        }

        // finish up everything on the main thread so that
        BSML::MainThreadScheduler::Schedule([this](){
            INFO("Finishing up level loading on main thread");
            RefreshLevelPacks();

            LevelsLoaded.invoke(_allLoadedLevels);

            // clear current future
            _currentlyLoadingFuture = std::future<void>();
        });
    }

    void RuntimeSongLoader::RefreshSongWorkerThread(std::mutex* levelsItrMutex, std::set<LevelPathAndWip>::const_iterator* levelsItr, std::set<LevelPathAndWip>::const_iterator* levelsEnd) {
        auto NextLevel = [](std::mutex& levelsItrMutex, std::set<LevelPathAndWip>::const_iterator& levelsItr, std::set<LevelPathAndWip>::const_iterator& levelsEnd) -> LevelPathAndWip {
            std::lock_guard<std::mutex> lock(levelsItrMutex);
            if (levelsItr != levelsEnd) {
                auto v = *levelsItr;
                levelsItr++;
                return v;
            } else {
                return {};
            }
        };

        while (*levelsItr != *levelsEnd) {
            auto [levelPath, isWip] = NextLevel(*levelsItrMutex, *levelsItr, *levelsEnd);

            // we got an invalid levelPath
            if (levelPath.empty()) {
                _loadedSongs++;
                continue;
            }

            try {
                time_point<high_resolution_clock> start = high_resolution_clock::now();
                GlobalNamespace::CustomPreviewBeatmapLevel* level = nullptr;
                StringW csLevelPath(levelPath.string());

                // pick the dictionary we need to add / check from based on whether this song is WIP
                auto targetDict = isWip ? _customWIPLevels : _customLevels;

                // preliminary check to see whether the song we are looking for already is in our dictionary
                bool containsKey = targetDict->ContainsKey(csLevelPath);
                if (containsKey) {
                    DEBUG("Level path {} exists in existing collection. WIP: {}", levelPath.string(), isWip);
                    level = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(targetDict->get_Item(levelPath.string()));
                }

                // if the level is not yet set, attempt loading levelinfosavedata from the song path, then load custom preview beatmap level from that
                if (!level) {
                    CustomJSONData::CustomLevelInfoSaveData* saveData = GetStandardSaveData(levelPath);
                    if (saveData) {
                        std::string hash;
                        level = LoadCustomPreviewBeatmapLevel(levelPath.string(), isWip, saveData, hash);
                    }
                }

                // if we now have a level, add it to the target dictionary, else log a failure
                if (level) {
                    targetDict->TryAdd(levelPath.string(), level);
                } else {
                    WARNING("Somehow failed to load song at path {}", levelPath.string());
                }

                INFO("Loaded song in {}ms", duration_cast<milliseconds>(high_resolution_clock::now() - start).count());

                // update progress
                _loadedSongs++;
                _loadProgress = (float)_loadedSongs / (float)_totalSongs;
            } catch (std::exception const& e) {
                ERROR("Caught exception of type {} while loading song @ path '{}', song will be skipped!", typeid(e).name(), levelPath.string());
                ERROR("Exception what: {}", e.what());
            } catch (...) {
                ERROR("Caught exception of unknown type (current_exception typeid: {}) while loading song @ path '{}', song will be skipped!", typeid(std::current_exception()).name(), levelPath.string());
            }
        }

        DEBUG("RuntimeSongLoader::RefreshSongWorkerThread end");
    }

    SongCore::CustomJSONData::CustomLevelInfoSaveData* RuntimeSongLoader::GetStandardSaveData(std::filesystem::path path) {
        DEBUG("RuntimeSongLoader::GetStandardSaveData");
        if (path.empty()) {
            ERROR("Provided path was empty!");
            return nullptr;
        }

        auto infoPath = path / "info.dat";
        if (!std::filesystem::exists(infoPath)) infoPath = path / "Info.dat";
        if (!std::filesystem::exists(infoPath)) {
            ERROR("no info.dat found for song @ '{}', returning null!", path.string());
            return nullptr;
        }

        try {
            auto standardSaveData = GlobalNamespace::StandardLevelInfoSaveData::DeserializeFromJSONString(Utils::ReadText(infoPath));
            DEBUG("standardSaveData: {}", standardSaveData->ToString());

            if (!standardSaveData) {
                ERROR("Cannot load file from path: {}!", path.string());
                return nullptr;
            }

            auto opt = il2cpp_utils::try_cast<SongCore::CustomJSONData::CustomLevelInfoSaveData>(standardSaveData);
            if (!opt.has_value()) {
                ERROR("Cannot load file {} as CustomLevelInfoSaveData!", path.string());
                return nullptr;
            }

            return opt.value();
        } catch(std::runtime_error& e) {
            ERROR("GetStandardSaveData can't Load File {}: {}!", path.string(), e.what());
        }

        // we errored out while doing things. return null
        return nullptr;
    }

    void RuntimeSongLoader::RefreshLevelPacks() {
        _customLevelPackCollection->ClearPacks();

        _customLevelPackCollection->AddPack(_customLevelPack);
        _customLevelPackCollection->AddPack(_customWIPLevelPack);

        CustomLevelPacksWillRefresh.invoke(_customLevelPackCollection);

        _beatmapLevelsModel->_customLevelPackCollection = _customLevelPackCollection->i___GlobalNamespace__IBeatmapLevelPackCollection();
        _beatmapLevelsModel->UpdateLoadedPreviewLevels();

        CustomLevelPacksRefreshed.invoke(_customLevelPackCollection);
    }

    GlobalNamespace::EnvironmentInfoSO* RuntimeSongLoader::GetEnvironmentInfo(StringW environmentName, bool allDirections) {
        auto env = _customLevelLoader->_environmentSceneInfoCollection->GetEnvironmentInfoBySerializedName(environmentName);
        if (!env)
            env = allDirections ? _customLevelLoader->_defaultAllDirectionsEnvironmentInfo : _customLevelLoader->_defaultEnvironmentInfo;
        return env;
    }

    ArrayW<GlobalNamespace::EnvironmentInfoSO*> RuntimeSongLoader::GetEnvironmentInfos(std::span<StringW const> environmentsNames) {
        if (environmentsNames.empty()) return ArrayW<GlobalNamespace::EnvironmentInfoSO*>::Empty();
        ListW<GlobalNamespace::EnvironmentInfoSO*> envs;

        for (auto environmentName : environmentsNames) {
            auto env = _customLevelLoader->_environmentSceneInfoCollection->GetEnvironmentInfoBySerializedName(environmentName);
            if (env) envs->Add(env);
        }

        return envs->ToArray();
    }

    ArrayW<GlobalNamespace::ColorScheme*> RuntimeSongLoader::GetColorSchemes(std::span<GlobalNamespace::BeatmapLevelColorSchemeSaveData* const> colorSchemeDatas) {
        if (colorSchemeDatas.empty()) return ArrayW<GlobalNamespace::ColorScheme*>::Empty();

        ListW<GlobalNamespace::ColorScheme*> colorSchemes = ListW<GlobalNamespace::ColorScheme*>::New();
        for (auto saveData : colorSchemeDatas) {
            auto colorScheme = saveData->colorScheme;
            if (colorScheme) {
                colorSchemes->Add(
                    GlobalNamespace::ColorScheme::New_ctor(
                        colorScheme->colorSchemeId,
                        "",
                        false,
                        "",
                        false,
                        colorScheme->saberAColor,
                        colorScheme->saberBColor,
                        colorScheme->environmentColor0,
                        colorScheme->environmentColor1,
                        {1, 1, 1, 1},
                        true,
                        colorScheme->environmentColor0Boost,
                        colorScheme->environmentColor1Boost,
                        {1, 1, 1, 1},
                        colorScheme->obstaclesColor
                    )
                );
            }
        }
        return colorSchemes->ToArray();
    }

    ListW<GlobalNamespace::PreviewDifficultyBeatmapSet*> RuntimeSongLoader::GetDifficultyBeatmapSets(std::span<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet* const> difficultyBeatmapSetDatas) {
        auto difficultyBeatmapSets = ListW<GlobalNamespace::PreviewDifficultyBeatmapSet*>::New();
        if (difficultyBeatmapSetDatas.empty()) return difficultyBeatmapSets;

        for (auto beatmapSetData : difficultyBeatmapSetDatas) {
            auto characteristic = _beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(beatmapSetData->beatmapCharacteristicName);

            if (characteristic) {
                ArrayW<GlobalNamespace::BeatmapDifficulty> difficulties(beatmapSetData->difficultyBeatmaps.size());

                for (int i = 0; auto& diff : difficulties) {
                    GlobalNamespace::BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(
                        beatmapSetData->difficultyBeatmaps[i++]->difficulty,
                        byref(diff)
                    );
                }

                difficultyBeatmapSets->Add(
                    GlobalNamespace::PreviewDifficultyBeatmapSet::New_ctor(
                        characteristic,
                        difficulties
                    )
                );
            } else {
                WARNING("Could not get a valid characteristic for name {}", beatmapSetData->beatmapCharacteristicName);
            }
        }

        return difficultyBeatmapSets;
    }

    StringW EmptyString() {
        static ConstString empty("");
        return empty;
    }

    #define FixEmptyString(name) \
    if(!name) { \
        WARNING("Fixed nullptr string \"{}\"! THIS SHOULDN'T HAPPEN!", #name); \
        name = EmptyString(); \
    }

    GlobalNamespace::CustomPreviewBeatmapLevel* RuntimeSongLoader::LoadCustomPreviewBeatmapLevel(std::filesystem::path levelPath, bool wip, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData, std::string& hashOut) {
        if (!saveData) {
            WARNING("saveData was null for level @ {}", levelPath.string());
            return nullptr;
        }
        auto hashOpt = Utils::GetCustomLevelHash(levelPath, saveData);
        hashOut = *hashOpt;

        std::string levelId = fmt::format("{}{}{}", CUSTOM_LEVEL_PREFIX_ID, hashOut, wip ? "" : " WIP");

        auto songName = saveData->songName;
        FixEmptyString(songName)
        auto songSubName = saveData->songSubName;
        FixEmptyString(songSubName);
        auto songAuthorName = saveData->songAuthorName;
        FixEmptyString(songAuthorName);
        auto levelAuthorName = saveData->levelAuthorName;
        FixEmptyString(levelAuthorName);

        auto bpm = saveData->beatsPerMinute;
        auto songTimeOffset = saveData->songTimeOffset;
        auto shuffle = saveData->shuffle;
        auto shufflePeriod = saveData->shufflePeriod;
        auto previewStartTime = saveData->previewStartTime;
        auto previewDuration = saveData->previewDuration;

        if (!saveData->environmentName) saveData->_environmentName = EmptyString();
        auto environmentInfo = GetEnvironmentInfo(saveData->environmentName, false);
        if (!saveData->allDirectionsEnvironmentName) saveData->_allDirectionsEnvironmentName = EmptyString();
        auto allDirectionsEnvironmentInfo = GetEnvironmentInfo(saveData->allDirectionsEnvironmentName, true);
        if (!saveData->environmentNames) saveData->_environmentNames = ArrayW<StringW>::Empty();
        auto environmentInfos = GetEnvironmentInfos(saveData->environmentNames);
        if (!saveData->colorSchemes) saveData->_colorSchemes = ArrayW<GlobalNamespace::BeatmapLevelColorSchemeSaveData*>::Empty();
        auto colorSchemes = GetColorSchemes(saveData->colorSchemes);
        if (!saveData->difficultyBeatmapSets) saveData->_difficultyBeatmapSets = ArrayW<GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmapSet*>::Empty();
        auto difficultyBeatmapSets = GetDifficultyBeatmapSets(saveData->difficultyBeatmapSets);

        auto result = GlobalNamespace::CustomPreviewBeatmapLevel::New_ctor(
            _customLevelLoader->_defaultPackCover.unsafePtr(),
            saveData,
            levelPath.string(),
            _cachedMediaAsyncLoader->i___GlobalNamespace__ISpriteAsyncLoader(),
            levelId,
            songName,
            songSubName,
            songAuthorName,
            levelAuthorName,
            bpm,
            songTimeOffset,
            shuffle,
            shufflePeriod,
            previewStartTime,
            previewDuration,
            environmentInfo,
            allDirectionsEnvironmentInfo,
            environmentInfos,
            colorSchemes,
            ::GlobalNamespace::PlayerSensitivityFlag::Unknown,
            difficultyBeatmapSets->i___System__Collections__Generic__IReadOnlyList_1_T_()
        );

        DEBUG("result: {}", fmt::ptr(result));
        return result;
    }

    std::filesystem::path RuntimeSongLoader::get_SongPath() const {
        return config.PreferredCustomLevelPath;
    }

    std::filesystem::path RuntimeSongLoader::get_WIPSongPath() const {
        return config.PreferredCustomWIPLevelPath;
    }
}
