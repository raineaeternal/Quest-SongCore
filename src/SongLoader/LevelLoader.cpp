#include "SongLoader/LevelLoader.hpp"
#include "CustomJSONData.hpp"
#include "SongLoader/RuntimeSongLoader.hpp"
#include "logging.hpp"
#include "Utils/Hashing.hpp"
#include "Utils/File.hpp"
#include "Utils/OggVorbis.hpp"

#include "GlobalNamespace/BeatmapDifficultySerializedMethods.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapLevelColorSchemeSaveData.hpp"
#include "GlobalNamespace/PlayerSaveData.hpp"
#include "GlobalNamespace/EnvironmentName.hpp"
#include "GlobalNamespace/BeatmapBasicData.hpp"

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

        float songDuration = 0.0f;
        std::string songFilePath = levelPath / static_cast<std::string>(saveData->get_songFilename());
        if (std::filesystem::exists(songFilePath)) { // only do this if the file exists
            songDuration = Utils::GetLengthFromOggVorbis(songFilePath);
            if (songDuration <= 0 || songDuration == std::numeric_limits<float>::infinity()) songDuration = 0.0f;
        }

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
        ListW<GlobalNamespace::EnvironmentInfoSO*> envs;

        for (auto environmentName : environmentsNames) {
            auto env = _environmentsListModel->GetEnvironmentInfoBySerializedName(environmentName);
            if (env) envs->Add(env);
        }

        return envs->ToArray();
    }

    ArrayW<GlobalNamespace::ColorScheme*> LevelLoader::GetColorSchemes(std::span<GlobalNamespace::BeatmapLevelColorSchemeSaveData* const> colorSchemeDatas) {
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
}
