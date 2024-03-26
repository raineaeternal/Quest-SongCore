#pragma once

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/IBeatmapLevelData.hpp"
#include "../CustomJSONData.hpp"

// type which is basically a beatmaplevel but one made by songcore, helps with identification
DECLARE_CLASS_CODEGEN(SongCore::SongLoader, CustomBeatmapLevel, GlobalNamespace::BeatmapLevel,
    DECLARE_CTOR(ctor,
        bool hasPrecalculatedData,
        ::StringW levelID,
        ::StringW songName,
        ::StringW songSubName,
        ::StringW songAuthorName,
        ::ArrayW<::StringW> allMappers,
        ::ArrayW<::StringW> allLighters,
        float_t beatsPerMinute,
        float_t integratedLufs,
        float_t songTimeOffset,
        float_t previewStartTime,
        float_t previewDuration,
        float_t songDuration,
        ::GlobalNamespace::PlayerSensitivityFlag contentRating,
        ::GlobalNamespace::IPreviewMediaData* previewMediaData,
        ::System::Collections::Generic::IReadOnlyDictionary_2<::System::ValueTuple_2<::UnityW<::GlobalNamespace::BeatmapCharacteristicSO>, ::GlobalNamespace::BeatmapDifficulty>, ::GlobalNamespace::BeatmapBasicData*>* beatmapBasicData
    );

    public:
        /// @brief path to the custom level
        std::string_view get_customLevelPath() const { return _customLevelPath; }
        __declspec(property(get=get_customLevelPath)) std::string_view customLevelPath;

        /// @brief level info.dat save data
        CustomJSONData::CustomLevelInfoSaveData* get_standardLevelInfoSaveData() const { return _customLevelSaveData; }
        __declspec(property(get=get_standardLevelInfoSaveData)) CustomJSONData::CustomLevelInfoSaveData* standardLevelInfoSaveData;

        /// @brief level beatmapleveldata
        GlobalNamespace::IBeatmapLevelData* get_beatmapLevelData() const { return _beatmapLevelData; }
        __declspec(property(get=get_beatmapLevelData)) GlobalNamespace::IBeatmapLevelData* beatmapLevelData;

        static CustomBeatmapLevel* New(
            std::string_view customLevelPath,
            CustomJSONData::CustomLevelInfoSaveData* saveData,
            GlobalNamespace::IBeatmapLevelData* beatmapLevelData,
            bool hasPrecalculatedData,
            ::StringW levelID,
            ::StringW songName,
            ::StringW songSubName,
            ::StringW songAuthorName,
            ::ArrayW<::StringW> allMappers,
            ::ArrayW<::StringW> allLighters,
            float_t beatsPerMinute,
            float_t integratedLufs,
            float_t songTimeOffset,
            float_t previewStartTime,
            float_t previewDuration,
            float_t songDuration,
            ::GlobalNamespace::PlayerSensitivityFlag contentRating,
            ::GlobalNamespace::IPreviewMediaData* previewMediaData,
            ::System::Collections::Generic::IReadOnlyDictionary_2<::System::ValueTuple_2<::UnityW<::GlobalNamespace::BeatmapCharacteristicSO>, ::GlobalNamespace::BeatmapDifficulty>, ::GlobalNamespace::BeatmapBasicData*>* beatmapBasicData
        );
    private:
        CustomJSONData::CustomLevelInfoSaveData* _customLevelSaveData;
        GlobalNamespace::IBeatmapLevelData* _beatmapLevelData;
        std::string _customLevelPath;
)
