#include "SongLoader/SongLoader.hpp"
#include "CustomJSONData.hpp"
#include "SongLoader/SongCoreCustomLevelPack.hpp"
#include "main.hpp"
#include "shared/SongCore.hpp"
#include "logging.hpp"
#include "config.hpp"
#include "paper/shared/utfcpp/source/utf8.h"

#include "Utils/File.hpp"

#include <chrono>
#include <clocale>
#include <exception>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <ostream>
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

        _customLevels = SongDict::New_ctor();
        _customWIPLevels = SongDict::New_ctor();
    }

    void RuntimeSongLoader::Awake() {
        RefreshSongs(true);
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
                    WARNING("Song path '{}' had no info.dat folder! skipping...", songPath.string());
                    continue;
                }

                out.emplace(songPath, isWip);
            }
        }
    }

    std::shared_future<void> RuntimeSongLoader::RefreshSongs(bool fullRefresh) {
        if (_currentlyLoadingFuture.valid()) {
            return _currentlyLoadingFuture;
        }
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

        std::vector<il2cpp_utils::il2cpp_aware_thread> workerThreads;
        workerThreads.reserve(MAX_THREAD_COUNT);

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

        // clear current future
        _currentlyLoadingFuture = std::future<void>();
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
            if (levelPath.empty()) continue;

            try {
                time_point<high_resolution_clock> start = high_resolution_clock::now();
                GlobalNamespace::CustomPreviewBeatmapLevel* level = nullptr;
                auto targetDict = isWip ? _customWIPLevels : _customLevels;
                bool containsKey = targetDict->ContainsKey(levelPath.string());
                auto customData = nullptr;
                if (containsKey) {
                    DEBUG("Level path {} exists in existing collection. WIP: {}", levelPath.string(), isWip);
                    level = reinterpret_cast<GlobalNamespace::CustomPreviewBeatmapLevel*>(targetDict->get_Item(levelPath.string()));
                } 
                
                // TODO: if not, attempt loading levelinfosavedata from the song path, then load custom preview beatmap level from that
                if (!level) {
                    auto saveData = GetStandardSaveData(levelPath.string());
                }
                
                // TODO: make sure that the custom data section is loaded by default, since it's always used anyway for things like requirements and custom diff names


                if (level) {
                    targetDict->TryAdd(levelPath.string(), level);
                }

                INFO("Time elapsed: {}ms", duration_cast<milliseconds>(high_resolution_clock::now() - start).count());
            } catch (std::exception const& e) {
                ERROR("Caught exception of type {} while loading song @ path '{}', song will be skipped!", typeid(e).name(), levelPath.string());
                ERROR("Exception what: {}", e.what());
            } catch (...) {
                ERROR("Caught exception of unknown type (current_exception typeid: {}) while loading song @ path '{}', song will be skipped!", typeid(std::current_exception()).name(), levelPath.string());
            }
        }
    }

    SongCore::CustomJSONData::CustomLevelInfoSaveData* RuntimeSongLoader::GetStandardSaveData(std::filesystem::path path) {
        if (path.empty()) {
            ERROR("Provided path was empty!");
            return nullptr; 
        }

        auto standardSaveData = GlobalNamespace::StandardLevelInfoSaveData::DeserializeFromJSONString(Utils::ReadText(path.string()));

        if (!standardSaveData) {
            ERROR("Cannot load file from path: {}!", path.string());
            return nullptr;
        }

        auto opt = il2cpp_utils::try_cast<SongCore::CustomJSONData::CustomLevelInfoSaveData>(standardSaveData);
        if (!opt.has_value()) {
            ERROR("Cannot load file {} as CustomLevelInfoSaveData!", path.string());
        }

        return opt.value();

        return nullptr;
    }

    GlobalNamespace::EnvironmentInfoSO* RuntimeSongLoader::LoadEnvironmentInfo(StringW environmentName, bool allDirections) {
        auto customLevelLoader = nullptr;
    }

    ArrayW<GlobalNamespace::EnvironmentInfoSO*> RuntimeSongLoader::LoadEnvironmentInfos(ArrayW<StringW> environmentName) {
        
    }

    ArrayW<GlobalNamespace::ColorScheme*> RuntimeSongLoader::LoadColorScheme(ArrayW<GlobalNamespace::BeatmapLevelColorSchemeSaveData*> colorSchemeDatas) {
        if (!colorSchemeDatas) return ArrayW<GlobalNamespace::ColorScheme*>::Empty();

        ListW<GlobalNamespace::ColorScheme*> colorSchemes = ListW<GlobalNamespace::ColorScheme*>::New();
        for (auto saveData : colorSchemeDatas) {
            auto colorScheme = reinterpret_cast<GlobalNamespace::ColorScheme*>(saveData->colorScheme);
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
    
    GlobalNamespace::CustomPreviewBeatmapLevel* RuntimeSongLoader::LoadCustomPreviewBeatmapLevel(std::string_view path, bool wip, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData, std::string& out) {

        std::string levelId = CUSTOM_LEVEL_PREFIX_ID + out;
        if (wip) {
            levelId += " WIP";
        }

        StringW songName = saveData->songName;
        StringW songSubName = saveData->songSubName;
        StringW songAuthorName = saveData->songAuthorName;
        StringW levelAuthorName = saveData->levelAuthorName;
        float bpm = saveData->beatsPerMinute;
        float songTimeOffset = saveData->songTimeOffset;
        float shuffle = saveData->shuffle;
        float shufflePeriod = saveData->shufflePeriod;
        float previewStartTime = saveData->previewStartTime;
        float previewDuration = saveData->previewDuration;


        auto result = GlobalNamespace::CustomPreviewBeatmapLevel::New_ctor(
            // LevelPackCover
            saveData,
            path,
            // MediaLoader
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
            // EnvInfo
            // AllDirectionsEnvInfo
            // EnvInfos
            // Colorscheme
            ::GlobalNamespace::PlayerSensitivityFlag::Unknown,
            //reinterpret_cast<IReadOnlyList_1<PreviewDifficultyBeatmapSet*>*>(list.convert());
        );

        return result;
    }

    void RuntimeSongLoader::Update() {

    }

    std::filesystem::path RuntimeSongLoader::get_SongPath() const {
        return config.PreferredCustomLevelPath;
    }

    std::filesystem::path RuntimeSongLoader::get_WIPSongPath() const {
        return config.PreferredCustomWIPLevelPath;
    }
}
