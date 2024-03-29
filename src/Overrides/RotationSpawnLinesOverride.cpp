#include "Overrides/RotationSpawnLinesOverride.hpp"
#include "SongLoader/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "CustomJSONData.hpp"
#include "SongLoader/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"

DEFINE_TYPE(SongCore::Overrides, RotationSpawnLinesOverride);

extern bool NoteSpawnLinesOverrideLevelIsCustom;
extern bool NoteSpawnLinesOverrideShowLines;

namespace SongCore::Overrides {

    void RotationSpawnLinesOverride::ctor(Zenject::DiContainer* container) {
        NoteSpawnLinesOverrideLevelIsCustom = false;
        NoteSpawnLinesOverrideShowLines = true;

        auto sceneSetupData = container->TryResolve<GlobalNamespace::GameplayCoreSceneSetupData*>();
        if (!sceneSetupData) return;

        auto customLevel = il2cpp_utils::try_cast<SongLoader::CustomBeatmapLevel>(sceneSetupData->beatmapLevel).value_or(nullptr);
        if (!customLevel) return;
        NoteSpawnLinesOverrideLevelIsCustom = true;

        auto saveData = customLevel->standardLevelInfoSaveData;
        if (!saveData) return;

        auto& beatmapKey = sceneSetupData->beatmapKey;
        auto characteristic = beatmapKey.beatmapCharacteristic;
        auto difficulty = beatmapKey.difficulty;

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
