#include "hooking.hpp"
#include "logging.hpp"
#include "config.hpp"

#include "GlobalNamespace/BeatmapObjectSpawnMovementData.hpp"

std::optional<float> NegativeNJSOverrideNoteJumpSpeed;

MAKE_AUTO_HOOK_MATCH(
    BeatmapObjectSpawnMovementData_Init,
    &GlobalNamespace::BeatmapObjectSpawnMovementData::Init,
    void,
    GlobalNamespace::BeatmapObjectSpawnMovementData* self,
    int32_t noteLinesCount,
    float_t startNoteJumpMovementSpeed,
    float_t startBpm,
    GlobalNamespace::BeatmapObjectSpawnMovementData::NoteJumpValueType noteJumpValueType,
    float_t noteJumpValue,
    GlobalNamespace::IJumpOffsetYProvider* jumpOffsetYProvider,
    UnityEngine::Vector3 rightVec,
    UnityEngine::Vector3 forwardVec
) {
    if (NegativeNJSOverrideNoteJumpSpeed.has_value() && NegativeNJSOverrideNoteJumpSpeed < 0) {
        startNoteJumpMovementSpeed = NegativeNJSOverrideNoteJumpSpeed.value();
    }

    BeatmapObjectSpawnMovementData_Init(
        self,
        noteLinesCount,
        startNoteJumpMovementSpeed,
        startBpm,
        noteJumpValueType,
        noteJumpValue,
        jumpOffsetYProvider,
        rightVec,
        forwardVec
    );
}
