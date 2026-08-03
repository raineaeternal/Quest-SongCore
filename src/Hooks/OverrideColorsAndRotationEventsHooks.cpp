#include "hooking.hpp"
#include "config.hpp"
#include "logging.hpp"

#include "GlobalNamespace/StandardLevelScenesTransitionSetupData.hpp"
#include "GlobalNamespace/MultiplayerLevelScenesTransitionSetupData.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "GlobalNamespace/EnvironmentsListModel.hpp"
#include "GlobalNamespace/ColorSchemeSO.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/BeatmapBasicData.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicExtensions.hpp"
#include "GlobalNamespace/RecordingToolManager.hpp"
#include "UnityEngine/Color.hpp"
#include "System/Nullable_1.hpp"
#include "System/ValueTuple_2.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"

#include "SongLoader/CustomBeatmapLevel.hpp"
#include "CustomJSONData.hpp"
#include "Characteristics.hpp"

/// @brief method to merge the given custom colors into a newly created color scheme, or nothing if all custom color overrides are disabled
GlobalNamespace::ColorScheme* ApplyOverrideColors(GlobalNamespace::ColorScheme* baseColorScheme, SongCore::CustomJSONData::CustomSaveDataInfo::BasicCustomDifficultyBeatmapDetails::CustomColors const& customColors);

GlobalNamespace::ColorScheme* GetOverrideColorScheme(GlobalNamespace::ColorScheme* baseColorScheme, SongCore::SongLoader::CustomBeatmapLevel* level, GlobalNamespace::BeatmapKey& beatmapKey);

// TODO: rotation events forcing (map-requested environmentType allDirections/default) is disabled for now.
// BeatmapCharacteristicExtensions::ContainsRotationEvents can't be hooked (too small, "Method cannot be
// hooked!" static_assert), and there's no mutable field to flip anymore since it's a computed static method.
// This feature has no PC SongCore equivalent either. Color overrides used to also be resolved in this same
// Init hook via a beatmapDatas dictionary lookup; that's now handled in the GameplayCoreSceneSetupData ctor
// hook below instead, since IBeatmapLevelData no longer exposes parsed BeatmapBasicData/beatmapColorScheme
// lookup by characteristic+difficulty.
/*
static bool RotationEventsOverrideActive = false;
static GlobalNamespace::BeatmapCharacteristic RotationEventsOverrideCharacteristic{};
static bool RotationEventsOverrideValue = false;

MAKE_AUTO_HOOK_MATCH(
    StandardLevelScenesTransitionSetupData_Init,
    &GlobalNamespace::StandardLevelScenesTransitionSetupData::Init, void,
    GlobalNamespace::StandardLevelScenesTransitionSetupData *self,
::StringW gameMode, ::by_ref<::GlobalNamespace::BeatmapKey> beatmapKey, ::GlobalNamespace::BeatmapLevel* beatmapLevel,
                   ::GlobalNamespace::OverrideEnvironmentSettings* overrideEnvironmentSettings, ::GlobalNamespace::ColorScheme* playerOverrideColorScheme, bool playerOverrideLightshowColors,
                   ::GlobalNamespace::GameplayModifiers* gameplayModifiers, ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings, ::GlobalNamespace::PracticeSettings* practiceSettings,
                   ::GlobalNamespace::EnvironmentsListModel* environmentsListModel, ::GlobalNamespace::AudioClipAsyncLoader* audioClipAsyncLoader, ::GlobalNamespace::SettingsManager* settingsManager,
                   ::GlobalNamespace::GameplayAdditionalInformation* gameplayAdditionalInformation, ::GlobalNamespace::BeatmapDataLoader* beatmapDataLoader,
                   ::GlobalNamespace::BeatmapLevelsEntitlementModel* beatmapLevelsEntitlementModel, ::GlobalNamespace::BeatmapLevelsModel* beatmapLevelsModel,
                   ::GlobalNamespace::IBeatmapLevelData* beatmapLevelData) {
    auto customLevel = i2c::try_cast<SongCore::SongLoader::CustomBeatmapLevel*>(beatmapLevel);
    if (!customLevel) {
        return StandardLevelScenesTransitionSetupData_Init(
            self,
            gameMode,
            beatmapKey,
            beatmapLevel,
            overrideEnvironmentSettings,
            playerOverrideColorScheme,
            playerOverrideLightshowColors,
            gameplayModifiers,
            playerSpecificSettings,
            practiceSettings,
            environmentsListModel,
            audioClipAsyncLoader,
            settingsManager,
            gameplayAdditionalInformation,
            beatmapDataLoader,
            beatmapLevelsEntitlementModel,
            beatmapLevelsModel,
            beatmapLevelData
        );
    }

    auto characteristic = beatmapKey->characteristic;
    auto diff = beatmapKey->difficulty;

    auto customSaveDataInfoOpt = customLevel->CustomSaveDataInfo;
    if (customSaveDataInfoOpt) {
        auto& customSaveDataInfo = customSaveDataInfoOpt->get();
        auto diffDetailsOpt = customSaveDataInfo.TryGetCharacteristicAndDifficulty(SongCore::API::Characteristics::GetCharacteristic(characteristic).serializedName, diff);
        if (diffDetailsOpt) {
            auto& diffDetails = diffDetailsOpt->get();
            // map requests rotation events to be enabled or not, so we do that here
            if (diffDetails.environmentType.has_value()) {
                auto& envType = diffDetails.environmentType.value();
                if (envType == "allDirections") {
                    RotationEventsOverrideActive = true;
                    RotationEventsOverrideCharacteristic = characteristic;
                    RotationEventsOverrideValue = true;
                } else if (envType == "default"){
                    RotationEventsOverrideActive = true;
                    RotationEventsOverrideCharacteristic = characteristic;
                    RotationEventsOverrideValue = false;
                }
            }
        }
    }

    StandardLevelScenesTransitionSetupData_Init(
        self,
        gameMode,
        beatmapKey,
        beatmapLevel,
        overrideEnvironmentSettings,
        playerOverrideColorScheme,
        playerOverrideLightshowColors,
        gameplayModifiers,
        playerSpecificSettings,
        practiceSettings,
        environmentsListModel,
        audioClipAsyncLoader,
        settingsManager,
        gameplayAdditionalInformation,
        beatmapDataLoader,
        beatmapLevelsEntitlementModel,
        beatmapLevelsModel,
        beatmapLevelData
    );

    RotationEventsOverrideActive = false;
}

// ContainsRotationEvents is a computed static method now rather than a mutable field on a live SO instance,
// so the "force rotation events on/off for this characteristic" override above is applied by intercepting it here
MAKE_AUTO_HOOK_MATCH(
    BeatmapCharacteristicExtensions_ContainsRotationEvents,
    &GlobalNamespace::BeatmapCharacteristicExtensions::ContainsRotationEvents, bool,
    GlobalNamespace::BeatmapCharacteristic characteristic
) {
    if (RotationEventsOverrideActive && characteristic.value__ == RotationEventsOverrideCharacteristic.value__) {
        return RotationEventsOverrideValue;
    }

    return BeatmapCharacteristicExtensions_ContainsRotationEvents(characteristic);
}
*/

// intercept the colorScheme right as it's assigned into the gameplay scene setup data, mirroring the PC
// approach of hooking set_colorScheme during Init, since IBeatmapLevelData no longer exposes a way to look up
// parsed BeatmapBasicData/beatmapColorScheme directly by characteristic+difficulty
MAKE_AUTO_HOOK_MATCH(
    GameplayCoreSceneSetupData_ctor,
    &GlobalNamespace::GameplayCoreSceneSetupData::_ctor, void,
    GlobalNamespace::GameplayCoreSceneSetupData* self,
    ::by_ref<::GlobalNamespace::BeatmapKey> beatmapKey, ::GlobalNamespace::BeatmapLevel* beatmapLevel, ::GlobalNamespace::GameplayModifiers* gameplayModifiers,
    ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings, ::GlobalNamespace::PracticeSettings* practiceSettings,
    ::GlobalNamespace::EnvironmentInfoSO* targetEnvironmentInfo, ::GlobalNamespace::EnvironmentInfoSO* originalEnvironmentInfo, ::GlobalNamespace::ColorScheme* colorScheme,
    ::GlobalNamespace::SettingsManager* settingsManager, ::GlobalNamespace::AudioClipAsyncLoader* audioClipAsyncLoader, ::GlobalNamespace::BeatmapDataLoader* beatmapDataLoader,
    ::GlobalNamespace::BeatmapLevelsEntitlementModel* beatmapLevelsEntitlementModel, bool enableBeatmapDataCaching, ::GlobalNamespace::EnvironmentsListModel* environmentsListModel,
    bool allowNullBeatmapLevelData, ::GlobalNamespace::BeatmapLevelsModel* beatmapLevelsModel, ::GlobalNamespace::IBeatmapLevelData* beatmapLevelData
) {
    auto customLevel = i2c::try_cast<SongCore::SongLoader::CustomBeatmapLevel*>(beatmapLevel);
    if (customLevel) {
        auto overrideColorScheme = GetOverrideColorScheme(colorScheme, customLevel, *beatmapKey);
        if (overrideColorScheme != nullptr) {
            colorScheme = overrideColorScheme;
        }
    }

    GameplayCoreSceneSetupData_ctor(
        self,
        beatmapKey,
        beatmapLevel,
        gameplayModifiers,
        playerSpecificSettings,
        practiceSettings,
        targetEnvironmentInfo,
        originalEnvironmentInfo,
        colorScheme,
        settingsManager,
        audioClipAsyncLoader,
        beatmapDataLoader,
        beatmapLevelsEntitlementModel,
        enableBeatmapDataCaching,
        environmentsListModel,
        allowNullBeatmapLevelData,
        beatmapLevelsModel,
        beatmapLevelData
    );
}

// Hooks and methods to fix override color scheme stuff

void FixupAndApplyColorScheme(GlobalNamespace::MultiplayerLevelScenesTransitionSetupData* self);

typedef ::System::ValueTuple_2<bool, ::GlobalNamespace::ColorScheme*> GetColorInfoType;
// TODO: FIX for multiplayer!!
// See git history for FixupAndApplyColorScheme
/*
MAKE_AUTO_HOOK_MATCH(
    MultiplayerLevelScenesTransitionSetupDataSO_InitColorInfo,
    &GlobalNamespace::MultiplayerLevelScenesTransitionSetupDataSO::InitColorInfo,
    void,
    GlobalNamespace::MultiplayerLevelScenesTransitionSetupDataSO* self,
    ::GlobalNamespace::ColorScheme* overrideColorScheme
) {
    MultiplayerLevelScenesTransitionSetupDataSO_InitColorInfo(self, overrideColorScheme);
    FixupAndApplyColorScheme(self);
}*/

static bool operator==(UnityEngine::Color lhs, UnityEngine::Color rhs) {
    return lhs.r == rhs.r &&
        lhs.g == rhs.g &&
        lhs.b == rhs.b &&
        lhs.a == rhs.a;
}


GlobalNamespace::ColorScheme* GetOverrideColorScheme(GlobalNamespace::ColorScheme* baseColorScheme, SongCore::SongLoader::CustomBeatmapLevel* level, GlobalNamespace::BeatmapKey& beatmapKey) {
    // if we're not allowed to apply any colors, don't do anything
    if (!config.customSongObstacleColors && !config.customSongEnvironmentColors && !config.customSongNoteColors) return nullptr;

    auto customSaveDataInfoOpt = level->CustomSaveDataInfo;
    if (!customSaveDataInfoOpt) return nullptr;
    auto& customSaveDataInfo = customSaveDataInfoOpt->get();

    auto characteristics = SongCore::Characteristics::get_instance();
    if (!characteristics) {
      WARNING(
          "Characteristics instance is null, cannot get override color scheme");
        return nullptr;
    };

    auto diffDetailsOpt = customSaveDataInfo.TryGetCharacteristicAndDifficulty(characteristics->GetCharacteristic(beatmapKey.characteristic).serializedName, beatmapKey.difficulty);
    if (!diffDetailsOpt.has_value()) return nullptr;
    auto& diffDetails = diffDetailsOpt->get();

    if (!diffDetails.customColors.has_value()) return nullptr;
    auto& customColors = diffDetails.customColors.value();

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

    // environment colors fall back to the map's note colors before falling back to the base scheme
    environmentColor0 = customColors.colorLeft.value_or(environmentColor0);
    environmentColor1 = customColors.colorRight.value_or(environmentColor1);

    if (config.customSongEnvironmentColors) {
        environmentColor0 = customColors.envColorLeft.value_or(environmentColor0);
        environmentColor1 = customColors.envColorRight.value_or(environmentColor1);
        environmentColorW = customColors.envColorWhite.value_or(environmentColorW);
        environmentColor0Boost = customColors.envColorLeftBoost.value_or(environmentColor0Boost);
        environmentColor1Boost = customColors.envColorRightBoost.value_or(environmentColor1Boost);
        environmentColorWBoost = customColors.envColorWhiteBoost.value_or(environmentColorWBoost);
    }

    UnityEngine::Color const defaultColor{};
    bool useOverrideBoostColors = !(environmentColor0Boost == defaultColor) && !(environmentColor1Boost == defaultColor);

    return GlobalNamespace::ColorScheme::New_ctor(
        "SongCoreOverrideColorScheme",
        "SongCoreOverrideColorScheme",
        true,
        "SongCoreOverrideColorScheme",
        false,
        true,
        saberAColor,
        saberBColor,
        true,
        environmentColor0,
        environmentColor1,
        environmentColorW,
        useOverrideBoostColors,
        environmentColor0Boost,
        environmentColor1Boost,
        environmentColorWBoost,
        obstaclesColor
    );
}
