#include "Overrides/RotationSpawnLinesOverride.hpp"
#include "SongLoader/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "CustomJSONData.hpp"
#include "SongLoader/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "Characteristics.hpp"

DEFINE_TYPE(SongCore::Overrides, RotationSpawnLinesOverride);

extern bool NoteSpawnLinesOverrideLevelIsCustom;
extern bool NoteSpawnLinesOverrideShowLines;

namespace SongCore::Overrides {

    void RotationSpawnLinesOverride::ctor(Zenject::DiContainer* container, SongCore::Characteristics* characteristics) {
        NoteSpawnLinesOverrideLevelIsCustom = false;
        NoteSpawnLinesOverrideShowLines = true;

        auto sceneSetupData = container->TryResolve<GlobalNamespace::GameplayCoreSceneSetupData*>();
        if (!sceneSetupData) return;

        auto customLevel = i2c::try_cast<SongLoader::CustomBeatmapLevel*>(sceneSetupData->beatmapLevel);
        if (!customLevel) return;
        NoteSpawnLinesOverrideLevelIsCustom = true;

        auto customSaveDataInfoOpt = customLevel->CustomSaveDataInfo;
        if (!customSaveDataInfoOpt) return;
        auto& customSaveDataInfo = customSaveDataInfoOpt->get();

        auto& beatmapKey = sceneSetupData->beatmapKey;
        auto difficulty = beatmapKey.difficulty;

        auto levelDetailsOpt = customSaveDataInfo.TryGetCharacteristicAndDifficulty(characteristics->GetCharacteristic(beatmapKey.characteristic).serializedName, difficulty);
        if (!levelDetailsOpt.has_value()) return;

        auto& levelDetails = levelDetailsOpt->get();
        NoteSpawnLinesOverrideShowLines = levelDetails.showRotationNoteSpawnLines.value_or(true);
    }

    void RotationSpawnLinesOverride::Dispose() {
        NoteSpawnLinesOverrideLevelIsCustom = false;
        NoteSpawnLinesOverrideShowLines = true;
    }
}
