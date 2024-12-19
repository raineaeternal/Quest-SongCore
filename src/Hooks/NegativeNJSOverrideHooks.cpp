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
    GlobalNamespace::IJumpOffsetYProvider* jumpOffsetYProvider,
    UnityEngine::Vector3 rightVec
) {

    // TODO: Port new patches https://github.com/Kylemc1413/SongCore/blob/master/source/SongCore/Patches/AllowNegativeNoteJumpSpeedPatch.cs

    BeatmapObjectSpawnMovementData_Init(
        self,
        noteLinesCount,
        jumpOffsetYProvider,
        rightVec
    );
}
