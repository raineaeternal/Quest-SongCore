#include "Overrides/NegativeNJSOverride.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapBasicData.hpp"

DEFINE_TYPE(SongCore::Overrides, NegativeNJSOverride);

extern std::optional<float> NegativeNJSOverrideNoteJumpSpeed;

namespace SongCore::Overrides {
    void NegativeNJSOverride::ctor(GlobalNamespace::GameplayCoreSceneSetupData* sceneSetupData) {
        auto beatmapKey = sceneSetupData->beatmapKey;
        auto difficultyData = sceneSetupData->beatmapLevel->GetDifficultyBeatmapData(beatmapKey.beatmapCharacteristic, beatmapKey.difficulty);
        NegativeNJSOverrideNoteJumpSpeed = difficultyData->noteJumpMovementSpeed;
    }

    void NegativeNJSOverride::Dispose() {
        NegativeNJSOverrideNoteJumpSpeed = std::nullopt;
    }
}
