#include "hooking.hpp"
#include "config.hpp"
#include "logging.hpp"

#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/EnvironmentsListModel.hpp"
#include "GlobalNamespace/ColorSchemeSO.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/RecordingToolManager.hpp"
#include "System/Nullable_1.hpp"

#include "SongLoader/CustomBeatmapLevel.hpp"
#include "CustomJSONData.hpp"

/// @brief method to merge the given custom colors into a newly created color scheme, or nothing if all custom color overrides are disabled
GlobalNamespace::ColorScheme* ApplyOverrideColors(GlobalNamespace::ColorScheme* baseColorScheme, SongCore::CustomJSONData::CustomLevelInfoSaveData::BasicCustomDifficultyBeatmapDetails::CustomColors const& customColors);

// TODO: extract similiar code into seperate method from both these hooks
MAKE_AUTO_HOOK_MATCH(
    StandardLevelScenesTransitionSetupDataSO_Init_1,
    static_cast<
        void
        (GlobalNamespace::StandardLevelScenesTransitionSetupDataSO::*)(
            ::StringW,
            ByRef<::GlobalNamespace::BeatmapKey>,
            ::GlobalNamespace::BeatmapLevel*,
            ::GlobalNamespace::OverrideEnvironmentSettings*,
            ::GlobalNamespace::ColorScheme*,
            ::GlobalNamespace::ColorScheme*,
            ::GlobalNamespace::GameplayModifiers*,
            ::GlobalNamespace::PlayerSpecificSettings*,
            ::GlobalNamespace::PracticeSettings*,
            ::GlobalNamespace::EnvironmentsListModel*,
            ::GlobalNamespace::AudioClipAsyncLoader*,
            ::GlobalNamespace::BeatmapDataLoader*,
            ::StringW,
            ::GlobalNamespace::BeatmapLevelsModel*,
            bool,
            bool,
            ::System::Nullable_1<::GlobalNamespace::__RecordingToolManager__SetupData>
    )>(&GlobalNamespace::StandardLevelScenesTransitionSetupDataSO::Init),
    void,
    GlobalNamespace::StandardLevelScenesTransitionSetupDataSO* self,
    ::StringW gameMode,
    ByRef<::GlobalNamespace::BeatmapKey> beatmapKey,
    ::GlobalNamespace::BeatmapLevel* beatmapLevel,
    ::GlobalNamespace::OverrideEnvironmentSettings* overrideEnvironmentSettings,
    ::GlobalNamespace::ColorScheme* overrideColorScheme,
    ::GlobalNamespace::ColorScheme* beatmapOverrideColorScheme,
    ::GlobalNamespace::GameplayModifiers* gameplayModifiers,
    ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings,
    ::GlobalNamespace::PracticeSettings* practiceSettings,
    ::GlobalNamespace::EnvironmentsListModel* environmentsListModel,
    ::GlobalNamespace::AudioClipAsyncLoader* audioClipAsyncLoader,
    ::GlobalNamespace::BeatmapDataLoader* beatmapDataLoader,
    ::StringW backButtonText,
    ::GlobalNamespace::BeatmapLevelsModel* beatmapLevelsModel,
    bool useTestNoteCutSoundEffects,
    bool startPaused,
    ::System::Nullable_1<::GlobalNamespace::__RecordingToolManager__SetupData> recordingToolData
) {
    auto customLevel = il2cpp_utils::try_cast<SongCore::SongLoader::CustomBeatmapLevel>(beatmapLevel).value_or(nullptr);
    if (!customLevel) {
        return StandardLevelScenesTransitionSetupDataSO_Init_1(
            self,
            gameMode,
            beatmapKey,
            beatmapLevel,
            overrideEnvironmentSettings,
            overrideColorScheme,
            beatmapOverrideColorScheme,
            gameplayModifiers,
            playerSpecificSettings,
            practiceSettings,
            environmentsListModel,
            audioClipAsyncLoader,
            beatmapDataLoader,
            backButtonText,
            beatmapLevelsModel,
            useTestNoteCutSoundEffects,
            startPaused,
            recordingToolData
        );
    }

    auto characteristic = beatmapKey->beatmapCharacteristic;
    auto diff = beatmapKey->difficulty;
    bool containsRotation = characteristic->containsRotationEvents;

    auto saveData = customLevel->standardLevelInfoSaveData;
    if (saveData) {
        auto diffDetailsOpt = saveData->TryGetCharacteristicAndDifficulty(characteristic->serializedName, diff);
        if (diffDetailsOpt) {
            auto& diffDetails = diffDetailsOpt->get();
            // apply custom color scheme if given
            if (diffDetails.customColors.has_value()) {
                // TODO: honor beatmap override color scheme n stuff
                if (overrideColorScheme) {
                    overrideColorScheme = ApplyOverrideColors(overrideColorScheme, diffDetails.customColors.value());
                } else {
                    auto envName = customLevel->GetEnvironmentName(characteristic, diff);
                    auto envInfo = environmentsListModel->GetEnvironmentInfoBySerializedNameSafe(envName._environmentName);
                    overrideColorScheme = ApplyOverrideColors(envInfo->colorScheme->colorScheme, diffDetails.customColors.value());
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

    StandardLevelScenesTransitionSetupDataSO_Init_1(
        self,
        gameMode,
        beatmapKey,
        beatmapLevel,
        overrideEnvironmentSettings,
        overrideColorScheme,
        beatmapOverrideColorScheme,
        gameplayModifiers,
        playerSpecificSettings,
        practiceSettings,
        environmentsListModel,
        audioClipAsyncLoader,
        beatmapDataLoader,
        backButtonText,
        beatmapLevelsModel,
        useTestNoteCutSoundEffects,
        startPaused,
        recordingToolData
    );

    characteristic->_containsRotationEvents = containsRotation;
}



MAKE_AUTO_HOOK_MATCH(
    StandardLevelScenesTransitionSetupDataSO_Init_2,
    static_cast<
        void
        (GlobalNamespace::StandardLevelScenesTransitionSetupDataSO::*)(
            ::StringW,
            ::GlobalNamespace::IBeatmapLevelData*,
            ByRef<::GlobalNamespace::BeatmapKey>,
            ::GlobalNamespace::BeatmapLevel*,
            ::GlobalNamespace::OverrideEnvironmentSettings*,
            ::GlobalNamespace::ColorScheme*,
            ::GlobalNamespace::ColorScheme*,
            ::GlobalNamespace::GameplayModifiers*,
            ::GlobalNamespace::PlayerSpecificSettings*,
            ::GlobalNamespace::PracticeSettings*,
            ::GlobalNamespace::EnvironmentsListModel*,
            ::GlobalNamespace::AudioClipAsyncLoader*,
            ::GlobalNamespace::BeatmapDataLoader*,
            ::StringW,
            bool,
            bool,
            ::System::Nullable_1<::GlobalNamespace::__RecordingToolManager__SetupData>
    )>(&GlobalNamespace::StandardLevelScenesTransitionSetupDataSO::Init),
    void,
    GlobalNamespace::StandardLevelScenesTransitionSetupDataSO* self,
    ::StringW gameMode,
    ::GlobalNamespace::IBeatmapLevelData* beatmapLevelData,
    ByRef<::GlobalNamespace::BeatmapKey> beatmapKey,
    ::GlobalNamespace::BeatmapLevel* beatmapLevel,
    ::GlobalNamespace::OverrideEnvironmentSettings* overrideEnvironmentSettings,
    ::GlobalNamespace::ColorScheme* overrideColorScheme,
    ::GlobalNamespace::ColorScheme* beatmapOverrideColorScheme,
    ::GlobalNamespace::GameplayModifiers* gameplayModifiers,
    ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings,
    ::GlobalNamespace::PracticeSettings* practiceSettings,
    ::GlobalNamespace::EnvironmentsListModel* environmentsListModel,
    ::GlobalNamespace::AudioClipAsyncLoader* audioClipAsyncLoader,
    ::GlobalNamespace::BeatmapDataLoader* beatmapDataLoader,
    ::StringW backButtonText,
    bool useTestNoteCutSoundEffects,
    bool startPaused,
    ::System::Nullable_1<::GlobalNamespace::__RecordingToolManager__SetupData> recordingToolData
) {
    auto customLevel = il2cpp_utils::try_cast<SongCore::SongLoader::CustomBeatmapLevel>(beatmapLevel).value_or(nullptr);
    if (!customLevel) {
        return StandardLevelScenesTransitionSetupDataSO_Init_2(
            self,
            gameMode,
            beatmapLevelData,
            beatmapKey,
            beatmapLevel,
            overrideEnvironmentSettings,
            overrideColorScheme,
            beatmapOverrideColorScheme,
            gameplayModifiers,
            playerSpecificSettings,
            practiceSettings,
            environmentsListModel,
            audioClipAsyncLoader,
            beatmapDataLoader,
            backButtonText,
            useTestNoteCutSoundEffects,
            startPaused,
            recordingToolData
        );
    }

    auto characteristic = beatmapKey->beatmapCharacteristic;
    auto diff = beatmapKey->difficulty;
    bool containsRotation = characteristic->containsRotationEvents;

    auto saveData = customLevel->standardLevelInfoSaveData;
    if (saveData) {
        auto diffDetailsOpt = saveData->TryGetCharacteristicAndDifficulty(characteristic->serializedName, diff);
        if (diffDetailsOpt) {
            auto& diffDetails = diffDetailsOpt->get();
            // apply custom color scheme if given
            if (diffDetails.customColors.has_value()) {
                // TODO: honor beatmap override color scheme n stuff
                if (overrideColorScheme) {
                    overrideColorScheme = ApplyOverrideColors(overrideColorScheme, diffDetails.customColors.value());
                } else {
                    auto envName = customLevel->GetEnvironmentName(characteristic, diff);
                    auto envInfo = environmentsListModel->GetEnvironmentInfoBySerializedNameSafe(envName._environmentName);
                    overrideColorScheme = ApplyOverrideColors(envInfo->colorScheme->colorScheme, diffDetails.customColors.value());
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

    StandardLevelScenesTransitionSetupDataSO_Init_2(
        self,
        gameMode,
        beatmapLevelData,
        beatmapKey,
        beatmapLevel,
        overrideEnvironmentSettings,
        overrideColorScheme,
        beatmapOverrideColorScheme,
        gameplayModifiers,
        playerSpecificSettings,
        practiceSettings,
        environmentsListModel,
        audioClipAsyncLoader,
        beatmapDataLoader,
        backButtonText,
        useTestNoteCutSoundEffects,
        startPaused,
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
