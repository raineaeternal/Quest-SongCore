#pragma once

#include "custom-types/shared/macros.hpp"
#include "CustomJSONData.hpp"
#include "CustomBeatmapLevel.hpp"

#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/BeatmapLevelColorSchemeSaveData.hpp"
#include "GlobalNamespace/CachedMediaAsyncLoader.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/AdditionalContentModel.hpp"
#include "GlobalNamespace/EnvironmentsListModel.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/FileSystemPreviewMediaData.hpp"
#include "GlobalNamespace/FileSystemBeatmapLevelData.hpp"
#include "GlobalNamespace/FileDifficultyBeatmap.hpp"
#include "System/ValueTuple_2.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include <filesystem>

DECLARE_CLASS_CODEGEN(SongCore::SongLoader, LevelLoader, System::Object,
    DECLARE_CTOR(ctor, GlobalNamespace::CachedMediaAsyncLoader* cachedMediaAsyncLoader, GlobalNamespace::BeatmapCharacteristicCollection* beatmapCharacteristicCollection, GlobalNamespace::IAdditionalContentModel* additionalContentModel, GlobalNamespace::EnvironmentsListModel* environmentsListModel);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::CachedMediaAsyncLoader*, _cachedMediaAsyncLoader);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::BeatmapCharacteristicCollection*, _beatmapCharacteristicCollection);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::AdditionalContentModel*, _additionalContentModel);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::EnvironmentsListModel*, _environmentsListModel);

    public:
        /// @brief gets the savedata from the path
        SongCore::CustomJSONData::CustomLevelInfoSaveData* GetStandardSaveData(std::filesystem::path const& path);

        /// @brief Loads songs at given path given
        /// @param path the path to the song
        /// @param isWip is this a wip song
        /// @param saveData the level save data, for custom levels this is always a custom level info savedata
        /// @param outHash output for the hash of this level, might be unneeded though
        /// @return loaded beatmap level, or nullptr if failed
        CustomBeatmapLevel* LoadCustomBeatmapLevel(std::filesystem::path const& levelPath, bool wip, SongCore::CustomJSONData::CustomLevelInfoSaveData* saveData, std::string& hashOut);

    private:
        using CharacteristicDifficultyPair = System::ValueTuple_2<UnityW<GlobalNamespace::BeatmapCharacteristicSO>, GlobalNamespace::BeatmapDifficulty>;
        using BeatmapBasicDataDict = System::Collections::Generic::Dictionary_2<CharacteristicDifficultyPair, GlobalNamespace::BeatmapBasicData*>;
        using BeatmapLevelDataDict = System::Collections::Generic::Dictionary_2<CharacteristicDifficultyPair, GlobalNamespace::FileDifficultyBeatmap*>;

        /// @brief preview media data from filesystem
        GlobalNamespace::FileSystemPreviewMediaData* GetPreviewMediaData(std::filesystem::path const& levelPath, CustomJSONData::CustomLevelInfoSaveData* saveData);

        /// @brief beatmap level data from filesystem & basic beatmap data from savedata
        std::pair<GlobalNamespace::FileSystemBeatmapLevelData*, BeatmapBasicDataDict*> GetBeatmapLevelAndBasicData(std::filesystem::path const& levelPath, std::string_view levelID, std::span<GlobalNamespace::EnvironmentName const> environmentNames, std::span<GlobalNamespace::ColorScheme* const> colorSchemes, CustomJSONData::CustomLevelInfoSaveData* saveData);

        /// @brief gets the environment info for the environmentName and whether it's all directions or not
        GlobalNamespace::EnvironmentInfoSO* GetEnvironmentInfo(StringW environmentName, bool allDirections);

        /// @brief gets the environmentinfos for the environmentNames
        ArrayW<GlobalNamespace::EnvironmentInfoSO*> GetEnvironmentInfos(std::span<StringW const> environmentsNames);

        /// @brief creates the color schemes for the savedata
        ArrayW<GlobalNamespace::ColorScheme*> GetColorSchemes(std::span<GlobalNamespace::BeatmapLevelColorSchemeSaveData* const> colorSchemeDatas);
)
