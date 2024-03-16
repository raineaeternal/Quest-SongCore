#include "hooking.hpp"
#include "config.hpp"
#include "logging.hpp"

#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/ColorSchemeSO.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "CustomJSONData.hpp"

/// @brief method to merge the given custom colors into a newly created color scheme, or nothing if all custom color overrides are disabled
GlobalNamespace::ColorScheme* ApplyOverrideColors(GlobalNamespace::ColorScheme* baseColorScheme, SongCore::CustomJSONData::CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails::CustomColors const& customColors);

MAKE_AUTO_HOOK_MATCH(StandardLevelScenesTransitionSetupDataSO_Init,
    &GlobalNamespace::StandardLevelScenesTransitionSetupDataSO::Init,
    void,
    GlobalNamespace::StandardLevelScenesTransitionSetupDataSO* self,
    ::StringW gameMode,
    ::GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap,
    ::GlobalNamespace::IPreviewBeatmapLevel* previewBeatmapLevel,
    ::GlobalNamespace::OverrideEnvironmentSettings* overrideEnvironmentSettings,
    ::GlobalNamespace::ColorScheme* overrideColorScheme,
    ::GlobalNamespace::ColorScheme* beatmapOverrideColorScheme,
    ::GlobalNamespace::GameplayModifiers* gameplayModifiers,
    ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings,
    ::GlobalNamespace::PracticeSettings* practiceSettings,
    ::StringW backButtonText,
    bool useTestNoteCutSoundEffects,
    bool startPaused,
    ::GlobalNamespace::BeatmapDataCache* beatmapDataCache,
    ::System::Nullable_1<::GlobalNamespace::__RecordingToolManager__SetupData> recordingToolData
) {
    auto customLevel = il2cpp_utils::try_cast<GlobalNamespace::CustomPreviewBeatmapLevel>(previewBeatmapLevel).value_or(nullptr);
    if (!customLevel) { // level was not custom
        return StandardLevelScenesTransitionSetupDataSO_Init(
            self,
            gameMode,
            difficultyBeatmap,
            previewBeatmapLevel,
            overrideEnvironmentSettings,
            overrideColorScheme,
            beatmapOverrideColorScheme,
            gameplayModifiers,
            playerSpecificSettings,
            practiceSettings,
            backButtonText,
            useTestNoteCutSoundEffects,
            startPaused,
            beatmapDataCache,
            recordingToolData
        );
    }

    auto characteristic = difficultyBeatmap->parentDifficultyBeatmapSet->beatmapCharacteristic;
    auto diff = difficultyBeatmap->difficulty;
    bool containsRotation = characteristic->containsRotationEvents;

    auto saveData = il2cpp_utils::try_cast<SongCore::CustomJSONData::CustomLevelInfoSaveData>(customLevel->standardLevelInfoSaveData).value_or(nullptr);
    if (saveData) {
        auto diffDetailsOpt = saveData->TryGetCharacteristicAndDifficulty(characteristic->serializedName, diff);
        if (diffDetailsOpt) {
            auto& diffDetails = diffDetailsOpt->get();
            // apply custom color scheme if given
            if (diffDetails.customColors.has_value()) {
                if (overrideColorScheme) {
                    overrideColorScheme = ApplyOverrideColors(overrideColorScheme, diffDetails.customColors.value());
                } else {
                    overrideColorScheme = ApplyOverrideColors(previewBeatmapLevel->environmentInfo->colorScheme->colorScheme, diffDetails.customColors.value());
                }
            }

            // map requests rotation events to be enabled or not, so we do that here
            if (diffDetails.environmentType.has_value()) {
                auto& envType = diffDetails.environmentType.value();
                if (envType == "allDirections") {
                    characteristic->_containsRotationEvents = true;
                } else if (envType == "default"){
                    characteristic->_containsRotationEvents = false;
                }
            }
        }
    }

    StandardLevelScenesTransitionSetupDataSO_Init(
        self,
        gameMode,
        difficultyBeatmap,
        previewBeatmapLevel,
        overrideEnvironmentSettings,
        overrideColorScheme,
        beatmapOverrideColorScheme,
        gameplayModifiers,
        playerSpecificSettings,
        practiceSettings,
        backButtonText,
        useTestNoteCutSoundEffects,
        startPaused,
        beatmapDataCache,
        recordingToolData
    );

    characteristic->_containsRotationEvents = containsRotation;
}

GlobalNamespace::ColorScheme* ApplyOverrideColors(GlobalNamespace::ColorScheme* baseColorScheme, SongCore::CustomJSONData::CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails::CustomColors const& customColors) {
    // if we're not allowed to apply any colors, don't do anything
    if (!config.customSongObstacleColors && !config.customSongEnvironmentColors && !config.customSongNoteColors) return baseColorScheme;

    // we just grab all colors by default
    UnityEngine::Color saberAColor = baseColorScheme->saberAColor;
    UnityEngine::Color saberBColor = baseColorScheme->saberBColor;
    UnityEngine::Color environmentColor0 = baseColorScheme->environmentColor0;
    UnityEngine::Color environmentColor1 = baseColorScheme->environmentColor1;
    UnityEngine::Color environmentColorW = baseColorScheme->environmentColorW;
    UnityEngine::Color environmentColor0Boost = baseColorScheme->environmentColor0Boost;
    UnityEngine::Color environmentColor1Boost = baseColorScheme->environmentColor1Boost;
    UnityEngine::Color environmentColorWBoost = baseColorScheme->environmentColorWBoost;
    UnityEngine::Color obstaclesColor = baseColorScheme->obstaclesColor;

    if (config.customSongObstacleColors) {
        obstaclesColor = customColors.obstacleColor.value_or(obstaclesColor);
    }

    if (config.customSongNoteColors) {
        saberAColor = customColors.colorLeft.value_or(saberAColor);
        saberBColor = customColors.colorRight.value_or(saberBColor);
    }

    if (config.customSongEnvironmentColors) {
        environmentColor0 = customColors.envColorLeft.value_or(environmentColor0);
        environmentColor1 = customColors.envColorRight.value_or(environmentColor1);
        environmentColorW = customColors.envColorWhite.value_or(environmentColorW);
        environmentColor0Boost = customColors.envColorLeftBoost.value_or(environmentColor0Boost);
        environmentColor1Boost = customColors.envColorRightBoost.value_or(environmentColor1Boost);
        environmentColorWBoost = customColors.envColorWhiteBoost.value_or(environmentColorWBoost);
    }

    return GlobalNamespace::ColorScheme::New_ctor(
        "SongCoreOverrideColorScheme",
        "SongCoreOverrideColorScheme",
        false,
        "SongCoreOverrideColorScheme",
        false,
        saberAColor,
        saberBColor,
        environmentColor0,
        environmentColor1,
        environmentColorW,
        baseColorScheme->supportsEnvironmentColorBoost,
        environmentColor0Boost,
        environmentColor1Boost,
        environmentColorWBoost,
        obstaclesColor
    );
}
