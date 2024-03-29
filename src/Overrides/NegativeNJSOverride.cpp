#include "Overrides/NegativeNJSOverride.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapBasicData.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"

DEFINE_TYPE(SongCore::Overrides, NegativeNJSOverride);

extern std::optional<float> NegativeNJSOverrideNoteJumpSpeed;

namespace SongCore::Overrides {
    void NegativeNJSOverride::ctor(Zenject::DiContainer* container) {
        NegativeNJSOverrideNoteJumpSpeed = std::nullopt;
        auto sceneSetupData = container->TryResolve<GlobalNamespace::GameplayCoreSceneSetupData*>();
        if (!sceneSetupData) return;

        auto beatmapKey = sceneSetupData->beatmapKey;
        auto difficultyData = sceneSetupData->beatmapLevel->GetDifficultyBeatmapData(beatmapKey.beatmapCharacteristic, beatmapKey.difficulty);
        NegativeNJSOverrideNoteJumpSpeed = difficultyData->noteJumpMovementSpeed;
    }

    void NegativeNJSOverride::Dispose() {
        NegativeNJSOverrideNoteJumpSpeed = std::nullopt;
    }
}
