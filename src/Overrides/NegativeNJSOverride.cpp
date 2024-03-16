#include "Overrides/NegativeNJSOverride.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"

DEFINE_TYPE(SongCore::Overrides, NegativeNJSOverride);

extern std::optional<float> NegativeNJSOverrideNoteJumpSpeed;

namespace SongCore::Overrides {
    void NegativeNJSOverride::ctor(GlobalNamespace::GameplayCoreSceneSetupData* sceneSetupData) {
        NegativeNJSOverrideNoteJumpSpeed = sceneSetupData->difficultyBeatmap->noteJumpMovementSpeed;
    }

    void NegativeNJSOverride::Dispose() {
        NegativeNJSOverrideNoteJumpSpeed = std::nullopt;
    }
}
