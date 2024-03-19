#include "SongLoader/CustomBeatmapLevel.hpp"

DEFINE_TYPE(SongCore::SongLoader, CustomBeatmapLevel);

namespace SongCore::SongLoader {
    void CustomBeatmapLevel::ctor(

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
    ) {
        INVOKE_CTOR();
        _ctor(
            hasPrecalculatedData,
            levelID,
            songName,
            songSubName,
            songAuthorName,
            allMappers,
            allLighters,
            beatsPerMinute,
            integratedLufs,
            songTimeOffset,
            previewStartTime,
            previewDuration,
            songDuration,
            contentRating,
            previewMediaData,
            beatmapBasicData
        );
    }

    CustomBeatmapLevel* CustomBeatmapLevel::New(
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
    ) {
        auto level = CustomBeatmapLevel::New_ctor(
            hasPrecalculatedData,
            levelID,
            songName,
            songSubName,
            songAuthorName,
            allMappers,
            allLighters,
            beatsPerMinute,
            integratedLufs,
            songTimeOffset,
            previewStartTime,
            previewDuration,
            songDuration,
            contentRating,
            previewMediaData,
            beatmapBasicData
        );

        level->_customLevelPath = customLevelPath;
        level->_customLevelSaveData = saveData;
        level->_beatmapLevelData = beatmapLevelData;

        return level;
    }
}
