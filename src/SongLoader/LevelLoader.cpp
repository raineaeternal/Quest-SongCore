#include "SongLoader/LevelLoader.hpp"
#include "CustomJSONData.hpp"
#include "SongLoader/RuntimeSongLoader.hpp"
#include "Utils/WavRiff.hpp"
#include "logging.hpp"
#include "Utils/Hashing.hpp"
#include "Utils/File.hpp"
#include "Utils/OggVorbis.hpp"
#include "Utils/WavRiff.hpp"
#include "Utils/Cache.hpp"

#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapLevelColorSchemeSaveData.hpp"
#include "GlobalNamespace/PlayerSaveData.hpp"
#include "GlobalNamespace/EnvironmentName.hpp"
#include "GlobalNamespace/BeatmapBasicData.hpp"
#include "GlobalNamespace/BpmTimeProcessor.hpp"
#include "UnityEngine/JsonUtility.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveData.hpp"
#include "BeatmapSaveDataVersion3/BeatmapSaveDataItem.hpp"
#include "utf8.h"
#include <cmath>
#include <exception>
#include <limits>

DEFINE_TYPE(SongCore::SongLoader, LevelLoader);

#define THROW_ON_MISSING_DATA

namespace SongCore::SongLoader {
    void LevelLoader::ctor(GlobalNamespace::CachedMediaAsyncLoader* cachedMediaAsyncLoader, GlobalNamespace::BeatmapCharacteristicCollection* beatmapCharacteristicCollection, GlobalNamespace::IAdditionalContentModel* additionalContentModel, GlobalNamespace::EnvironmentsListModel* environmentsListModel) {
        INVOKE_CTOR();
        _cachedMediaAsyncLoader = cachedMediaAsyncLoader;
        _beatmapCharacteristicCollection = beatmapCharacteristicCollection;
        _additionalContentModel = il2cpp_utils::try_cast<GlobalNamespace::AdditionalContentModel>(additionalContentModel).value_or(nullptr);
        _environmentsListModel = environmentsListModel;
        _clipLoader = GlobalNamespace::AudioClipAsyncLoader::CreateDefault();
    }

    SongCore::CustomJSONData::CustomLevelInfoSaveData* LevelLoader::GetStandardSaveData(std::filesystem::path const& path) {
        if (path.empty()) {
            ERROR("Provided path was empty!");
            return nullptr;
        }

        auto infoPath = path / "info.dat";
        if (!std::filesystem::exists(infoPath)) {
            infoPath = path / "Info.dat";
            if (!std::filesystem::exists(infoPath)) {
                ERROR("no info.dat found for song @ '{}', returning null!", path.string());
                return nullptr;
            }
        }

        try {
            auto standardSaveData = GlobalNamespace::StandardLevelInfoSaveData::DeserializeFromJSONString(Utils::ReadText(infoPath));

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

    StringW EmptyString() {
        static ConstString empty("");
        return empty;
    }

    #define FixEmptyString(name) \
    if(!name) { \
        name = EmptyString(); \
    }

    CustomBeatmapLevel* LevelLoader::LoadCustomBeatmapLevel(std::filesystem::path const& levelPath, bool wip, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData, std::string& hashOut) {
        if (!saveData) {
            WARNING("saveData was null for level @ {}", levelPath.string());
            return nullptr;
        }

        auto hashOpt = Utils::GetCustomLevelHash(levelPath, saveData);
        hashOut = *hashOpt;

        std::string levelId = fmt::format("{}{}{}", RuntimeSongLoader::CUSTOM_LEVEL_PREFIX_ID, hashOut, wip ? " WIP" : "");

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

        float songDuration = GetLengthForLevel(levelPath, saveData);

        std::vector<GlobalNamespace::EnvironmentName> environmentNameList;
        if (environmentInfos.size() == 0) {
            environmentNameList.emplace_back(
                environmentInfo->serializedName
            );
            environmentNameList.emplace_back(
                allDirectionsEnvironmentInfo->serializedName
            );
        } else {
            for (auto info : environmentInfos) {
                environmentNameList.emplace_back(
                    info->serializedName
                );
            }
        }

        auto allMappers = ArrayW<StringW>({ levelAuthorName });
        auto allLighters = ArrayW<StringW>::Empty();

        auto previewMediaData = GetPreviewMediaData(levelPath, saveData);
        auto [beatmapLevelData, beatmapBasicData] = GetBeatmapLevelAndBasicData(levelPath, levelId, environmentNameList, colorSchemes, saveData);

        auto result = CustomBeatmapLevel::New(
            levelPath.string(),
            saveData,
            beatmapLevelData->i___GlobalNamespace__IBeatmapLevelData(),
            false,
            levelId,
            songName,
            songSubName,
            songAuthorName,
            allMappers,
            allLighters,
            bpm,
            -6.0f,
            songTimeOffset,
            previewStartTime,
            previewDuration,
            songDuration,
            GlobalNamespace::PlayerSensitivityFlag::Safe,
            previewMediaData->i___GlobalNamespace__IPreviewMediaData(),
            beatmapBasicData->i___System__Collections__Generic__IReadOnlyDictionary_2_TKey_TValue_()
        );

        return result;
    }

    std::pair<GlobalNamespace::FileSystemBeatmapLevelData*, LevelLoader::BeatmapBasicDataDict*> LevelLoader::GetBeatmapLevelAndBasicData(std::filesystem::path const& levelPath, std::string_view levelID, std::span<GlobalNamespace::EnvironmentName const> environmentNames, std::span<GlobalNamespace::ColorScheme* const> colorSchemes, CustomJSONData::CustomLevelInfoSaveData* saveData) {
        auto fileDifficultyBeatmapsDict = System::Collections::Generic::Dictionary_2<CharacteristicDifficultyPair, GlobalNamespace::FileDifficultyBeatmap*>::New_ctor();
        auto basicDataDict = LevelLoader::BeatmapBasicDataDict::New_ctor();
        bool saveDataHadEnvNames = saveData->environmentNames.size() > 0;

        for (auto beatmapSet : saveData->difficultyBeatmapSets) {
            auto characteristic = _beatmapCharacteristicCollection->GetBeatmapCharacteristicBySerializedName(beatmapSet->beatmapCharacteristicName);
            if (!characteristic) {
                #ifdef THROW_ON_MISSING_DATA
                    throw std::runtime_error(fmt::format("Got null characteristic for characteristic name {}", beatmapSet->beatmapCharacteristicName));
                #else
                    WARNING("Got null characteristic for characteristic name {}, skipping...", beatmapSet->beatmapCharacteristicName);
                    continue;
                #endif
            }

            for (auto difficultyBeatmap : beatmapSet->difficultyBeatmaps) {
                GlobalNamespace::BeatmapDifficulty difficulty;
                auto parseSuccess = GlobalNamespace::BeatmapDifficultySerializedMethods::BeatmapDifficultyFromSerializedName(
                    difficultyBeatmap->difficulty,
                    byref(difficulty)
                );

                if (!parseSuccess) {
                    #ifdef THROW_ON_MISSING_DATA
                        throw std::runtime_error(fmt::format("Failed to parse a diff string: {}", difficultyBeatmap->difficulty));
                    #else
                        WARNING("Failed to parse a diff string: {}, skipping...", difficultyBeatmap->difficulty);
                        continue;
                    #endif
                }

                auto beatmapPath = levelPath / std::string(difficultyBeatmap->beatmapFilename);
                if (!std::filesystem::exists(beatmapPath)) {
                    #ifdef THROW_ON_MISSING_DATA
                        throw std::runtime_error(fmt::format("Diff file '{}' does not exist", beatmapPath.string()));
                    #else
                        WARNING("Diff file '{}' does not exist, skipping...", beatmapPath.string());
                        continue;
                    #endif
                }

                auto const dictKey = CharacteristicDifficultyPair(
                    characteristic,
                    difficulty
                );

                fileDifficultyBeatmapsDict->Add(
                    dictKey,
                    GlobalNamespace::FileDifficultyBeatmap::New_ctor(
                        "",
                        beatmapPath.string(),
                        ""
                    )
                );

                // if we have env names, use the idx, otherwise use whether the char had rotation (no rot means use default env, otherwise use rotation env)
                int envNameIndex = saveDataHadEnvNames ? difficultyBeatmap->environmentNameIdx : characteristic->containsRotationEvents ? 1 : 0;
                envNameIndex = std::clamp<int>(envNameIndex, 0, environmentNames.size());
                int colorSchemeIndex = difficultyBeatmap->beatmapColorSchemeIdx;
                auto colorScheme = (colorSchemeIndex >= 0 && colorSchemeIndex < colorSchemes.size()) ? colorSchemes[colorSchemeIndex] : nullptr;

                basicDataDict->Add(
                    dictKey,
                    GlobalNamespace::BeatmapBasicData::New_ctor(
                        difficultyBeatmap->noteJumpMovementSpeed,
                        difficultyBeatmap->noteJumpStartBeatOffset,
                        environmentNames[envNameIndex],
                        colorScheme,
                        0,
                        0,
                        0,
                        ArrayW<StringW>::Empty(),
                        ArrayW<StringW>::Empty()
                    )
                );
            }
        }

        return {
            GlobalNamespace::FileSystemBeatmapLevelData::New_ctor(
                levelID,
                (levelPath / std::string(saveData->songFilename)).string(),
                "",
                fileDifficultyBeatmapsDict
            ),
            basicDataDict
        };
    }

    GlobalNamespace::FileSystemPreviewMediaData* LevelLoader::GetPreviewMediaData(std::filesystem::path const& levelPath, CustomJSONData::CustomLevelInfoSaveData* saveData) {
        return GlobalNamespace::FileSystemPreviewMediaData::New_ctor(
            _cachedMediaAsyncLoader->i___GlobalNamespace__ISpriteAsyncLoader(),
            _clipLoader,
            levelPath.string(),
            saveData->coverImageFilename,
            saveData->songFilename
        );
    }

    GlobalNamespace::EnvironmentInfoSO* LevelLoader::GetEnvironmentInfo(StringW environmentName, bool allDirections) {
        auto env = _environmentsListModel->GetEnvironmentInfoBySerializedName(environmentName);
        if (!env)
            env = _environmentsListModel->GetFirstEnvironmentInfoWithType(allDirections ? GlobalNamespace::EnvironmentType::Circle : GlobalNamespace::EnvironmentType::Normal);
        return env;
    }

    ArrayW<GlobalNamespace::EnvironmentInfoSO*> LevelLoader::GetEnvironmentInfos(std::span<StringW const> environmentsNames) {
        if (environmentsNames.empty()) return ArrayW<GlobalNamespace::EnvironmentInfoSO*>::Empty();
        auto envs = ListW<GlobalNamespace::EnvironmentInfoSO*>::New();

        for (auto environmentName : environmentsNames) {
            auto env = _environmentsListModel->GetEnvironmentInfoBySerializedName(environmentName);
            if (env) envs->Add(env);
        }

        return envs->ToArray();
    }

    ArrayW<GlobalNamespace::ColorScheme*> LevelLoader::GetColorSchemes(std::span<GlobalNamespace::BeatmapLevelColorSchemeSaveData* const> colorSchemeDatas) {
        if (colorSchemeDatas.empty()) return ArrayW<GlobalNamespace::ColorScheme*>::Empty();

        auto colorSchemes = ListW<GlobalNamespace::ColorScheme*>::New();
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

    float LevelLoader::GetLengthForLevel(std::filesystem::path const& levelPath, CustomJSONData::CustomLevelInfoSaveData* saveData) {
        // check the cached info
        auto cachedInfoOpt = Utils::GetCachedInfo(levelPath);
        if (cachedInfoOpt.has_value() && cachedInfoOpt->songDuration.has_value()) {
            return cachedInfoOpt->songDuration.value();
        } else {
            // try to get the info from the ogg file
            std::string songFilePath = levelPath / static_cast<std::string>(saveData->songFilename);
            if (std::filesystem::exists(songFilePath)) {
                float songDuration = Utils::GetLengthFromOggVorbis(songFilePath);
                if (songDuration >= 0 && !std::isnan(songDuration)) { // found duration was valid
                    // update cache with new duration
                    auto info = cachedInfoOpt.value_or(Utils::CachedSongData());
                    info.songDuration = songDuration;
                    Utils::SetCachedInfo(levelPath, info);
                    return songDuration;
                }
                songDuration = Utils::GetLengthFromWavRiff(songFilePath);
                if (songDuration >= 0 && !std::isnan(songDuration)) { // found duration was valid
                    // update cache with new duration
                    auto info = cachedInfoOpt.value_or(Utils::CachedSongData());
                    info.songDuration = songDuration;
                    Utils::SetCachedInfo(levelPath, info);
                    return songDuration;
                }
            }

            // if the file didn't exist or we didn't get a valid length from the ogg, we go and get it from the map
            float songDuration = GetLengthFromMap(levelPath, saveData);
            auto info = cachedInfoOpt.value_or(Utils::CachedSongData());
            info.songDuration = songDuration;
            Utils::SetCachedInfo(levelPath, info);
            return songDuration;
        }
    }

    float LevelLoader::GetLengthFromMap(std::filesystem::path const& levelPath, CustomJSONData::CustomLevelInfoSaveData* saveData) {
        try {
            static auto GetFirstAvailableDiffFile = [](std::filesystem::path const& levelPath, CustomJSONData::CustomLevelInfoSaveData* saveData) -> GlobalNamespace::StandardLevelInfoSaveData::DifficultyBeatmap* {
                for (auto set : saveData->difficultyBeatmapSets) {
                    auto beatmaps = set->difficultyBeatmaps;
                    for (auto itr = beatmaps.rbegin(); itr != beatmaps.rend(); itr ++) {
                        std::string fileName((*itr)->beatmapFilename);
                        if (!fileName.empty() && std::filesystem::exists(levelPath / fileName)) {
                            return *itr;
                        }
                    }
                }

                return nullptr;
            };

            auto diff = GetFirstAvailableDiffFile(levelPath, saveData);
            if (!diff) {
                WARNING("No diff files were found for level {} to get beatmap length", levelPath.string());
                return 0;
            }

            auto beatmapFilePath = levelPath / std::string(diff->beatmapFilename);
            auto saveDataString = Utils::ReadText(beatmapFilePath);
            auto beatmapSaveData = UnityEngine::JsonUtility::FromJson<BeatmapSaveDataVersion3::BeatmapSaveData*>(saveDataString);

            if (!beatmapSaveData) {
                WARNING("Could not parse beatmap savedata for time from savedata string {}", utf8::utf16to8(saveDataString));
                return 0;
            }

            float highestBeat = 0.0f;
            if (beatmapSaveData->colorNotes && beatmapSaveData->colorNotes->Count > 0) {
                for (auto note : ListW<::BeatmapSaveDataVersion3::ColorNoteData*>(beatmapSaveData->colorNotes)) {
                    auto beat = note->beat;
                    if (beat > highestBeat) highestBeat = beat;
                }
            } else if (beatmapSaveData->basicBeatmapEvents && beatmapSaveData->basicBeatmapEvents->Count > 0) {
                for (auto event : ListW<::BeatmapSaveDataVersion3::BasicEventData*>(beatmapSaveData->basicBeatmapEvents)) {
                    auto beat = event->beat;
                    if (beat > highestBeat) highestBeat = beat;
                }
            }

            auto bmpInTimeProcessor = GlobalNamespace::BpmTimeProcessor::New_ctor(saveData->beatsPerMinute, beatmapSaveData->bpmEvents->i___System__Collections__Generic__IReadOnlyList_1_T_());
            return bmpInTimeProcessor->ConvertBeatToTime(highestBeat);
        } catch (std::exception const& e) {
            ERROR("While determining length from map, caught exception {}: {}", typeid(e).name(), e.what());
        } catch (...) {
            ERROR("While determining length from map, caught error {} with no known what()!", typeid(std::current_exception()).name());
        }

        WARNING("Getting length from map failed, so duration will be 0");
        // we somehow failed, so we return 0
        return 0;
    }

}
