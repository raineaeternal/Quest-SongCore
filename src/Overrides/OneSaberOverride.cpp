#include "Overrides/OneSaberOverride.hpp"
#include "SongLoader/CustomBeatmapLevel.hpp"
#include "CustomJSONData.hpp"
#include "Characteristics.hpp"
#include "config.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"

DEFINE_TYPE(SongCore::Overrides, OneSaberOverride);

extern bool OneSaberOverrideActive;
extern bool OneSaberOverrideForceOneSaber;

namespace SongCore::Overrides {

    void OneSaberOverride::ctor(Zenject::DiContainer* container, SongCore::Characteristics* characteristics) {
        OneSaberOverrideActive = false;

        if (config.disableOneSaberOverride) return;

        auto sceneSetupData = container->TryResolve<GlobalNamespace::GameplayCoreSceneSetupData*>();
        if (!sceneSetupData) return;

        auto customLevel = i2c::try_cast<SongLoader::CustomBeatmapLevel*>(sceneSetupData->beatmapLevel);
        if (!customLevel) return;

        auto customSaveDataInfoOpt = customLevel->CustomSaveDataInfo;
        if (!customSaveDataInfoOpt) return;
        auto& customSaveDataInfo = customSaveDataInfoOpt->get();

        auto& beatmapKey = sceneSetupData->beatmapKey;
        auto difficultyDataOpt = customSaveDataInfo.TryGetCharacteristicAndDifficulty(characteristics->GetCharacteristic(beatmapKey.characteristic)->serializedName, beatmapKey.difficulty);
        if (!difficultyDataOpt.has_value()) return;

        auto& difficultyData = difficultyDataOpt->get();
        if (!difficultyData.oneSaber.has_value()) return;

        OneSaberOverrideActive = true;
        OneSaberOverrideForceOneSaber = difficultyData.oneSaber.value();
    }

    void OneSaberOverride::Dispose() {
        OneSaberOverrideActive = false;
    }
}
