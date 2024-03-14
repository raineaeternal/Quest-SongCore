#include "Overrides/RotationSpawnLinesOverride.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "CustomJSONData.hpp"

DEFINE_TYPE(SongCore::Overrides, RotationSpawnLinesOverride);

extern bool NoteSpawnLinesOverrideLevelIsCustom;
extern bool NoteSpawnLinesOverrideShowLines;

namespace SongCore::Overrides {

    void RotationSpawnLinesOverride::ctor(GlobalNamespace::GameplayCoreSceneSetupData* sceneSetupData) {
        _sceneSetupData = sceneSetupData;
    }

    void RotationSpawnLinesOverride::Initialize() {
        NoteSpawnLinesOverrideLevelIsCustom = false;
        NoteSpawnLinesOverrideShowLines = true;
        auto customPreviewLevel = il2cpp_utils::try_cast<GlobalNamespace::CustomPreviewBeatmapLevel>(_sceneSetupData->previewBeatmapLevel).value_or(nullptr);
        if (!customPreviewLevel) return;

        auto saveData = il2cpp_utils::try_cast<CustomJSONData::CustomLevelInfoSaveData>(customPreviewLevel->standardLevelInfoSaveData).value_or(nullptr);
        if (!saveData) return;

        auto difficultyBeatmap = _sceneSetupData->difficultyBeatmap;
        auto characteristic = difficultyBeatmap->parentDifficultyBeatmapSet->beatmapCharacteristic;
        auto difficulty = difficultyBeatmap->difficulty;

        auto levelDetailsOpt = saveData->TryGetCharacteristicAndDifficulty(characteristic->serializedName, difficulty);
        if (!levelDetailsOpt.has_value()) return;

        auto& levelDetails = levelDetailsOpt->get();
        NoteSpawnLinesOverrideShowLines = levelDetails.showRotationNoteSpawnLines.value_or(true);
    }

    void RotationSpawnLinesOverride::Dispose() {
        NoteSpawnLinesOverrideLevelIsCustom = false;
        NoteSpawnLinesOverrideShowLines = true;
    }
}
