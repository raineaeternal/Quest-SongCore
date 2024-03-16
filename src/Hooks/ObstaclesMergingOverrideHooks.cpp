#include "hooking.hpp"
#include "logging.hpp"

#include "GlobalNamespace/BeatmapDataTransformHelper.hpp"
#include "GlobalNamespace/BoolSO.hpp"
#include "GlobalNamespace/MainSettingsModelSO.hpp"

#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"

// hook to get around the `IsObstaclesMergingNeeded` if statement in this method. if it's a custom level we want to block merging, and since the if statement checks for !displacementEnabled we just set that to true
MAKE_AUTO_HOOK_MATCH(
    BeatmapDataTransformHelper_CreateTransformedBeatmapData,
    &GlobalNamespace::BeatmapDataTransformHelper::CreateTransformedBeatmapData,
    GlobalNamespace::IReadonlyBeatmapData*,
    GlobalNamespace::IReadonlyBeatmapData* beatmapData,
    GlobalNamespace::IPreviewBeatmapLevel* beatmapLevel,
    GlobalNamespace::GameplayModifiers* gameplayModifiers,
    bool leftHanded,
    GlobalNamespace::EnvironmentEffectsFilterPreset environmentEffectsFilterPreset,
    GlobalNamespace::EnvironmentIntensityReductionOptions* environmentIntensityReductionOptions,
    GlobalNamespace::MainSettingsModelSO* mainSettingsModel
) {
    bool wasEnabled = mainSettingsModel->screenDisplacementEffectsEnabled->_value;

    if (beatmapLevel->levelID.starts_with(u"custom_level_"))
        mainSettingsModel->screenDisplacementEffectsEnabled->____value = true;

    auto result = BeatmapDataTransformHelper_CreateTransformedBeatmapData(
        beatmapData,
        beatmapLevel,
        gameplayModifiers,
        leftHanded,
        environmentEffectsFilterPreset,
        environmentIntensityReductionOptions,
        mainSettingsModel
    );

    mainSettingsModel->screenDisplacementEffectsEnabled->____value = wasEnabled;

    return result;
}
