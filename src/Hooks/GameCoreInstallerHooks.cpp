#include <GlobalNamespace/VariableMovementDataProvider.hpp>

#include "Characteristics.hpp"
#include "hooking.hpp"
#include "logging.hpp"
#include "config.hpp"

#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapBasicData.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnController.hpp"
#include "SongLoader/CustomBeatmapLevel.hpp"
#include "CustomJSONData.hpp"
#include "Zenject/DiContainer.hpp"


void PostInstallBindings(void(*GameplayCoreInstaller_InstallBindings)(GlobalNamespace::GameplayCoreInstaller* self), GlobalNamespace::GameplayCoreInstaller* instance);

// GameplayCoreInstaller::InstallBindings
// PREFIX
    // OneSaberOverride
    // sets characteristic->_numberOfColors to saberCount
// POSTFIX
   // NegativeNJS patch
   // Fetches negative NJS from difficultyBeatmapData and updates to BeatmapObjectSpawnController::InitData
MAKE_AUTO_HOOK_MATCH(GameplayCoreInstaller_InstallBindings, &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void, GlobalNamespace::GameplayCoreInstaller* self) {
    // if override is disabledl, just run orig and nothing else
    if (config.disableOneSaberOverride) return PostInstallBindings(GameplayCoreInstaller_InstallBindings, self);

    auto sceneSetupData = self->_sceneSetupData;
    // get custom preview level
    auto level = i2c::try_cast<SongCore::SongLoader::CustomBeatmapLevel*>(sceneSetupData->beatmapLevel);
    if (!level) return GameplayCoreInstaller_InstallBindings(self);

    // get custom level save data if possible
    auto customSaveDataInfoOpt = level->CustomSaveDataInfo;
    if (!customSaveDataInfoOpt) return PostInstallBindings(GameplayCoreInstaller_InstallBindings, self);
    auto& customSaveDataInfo = customSaveDataInfoOpt->get();

    // get difficulty data
    auto beatmapKey = sceneSetupData->beatmapKey;
    // TODO: Get CharacteristicSO from somewhere
    auto characteristic = beatmapKey.characteristic;
    auto difficulty = beatmapKey.difficulty;
    auto difficultyDataOpt = customSaveDataInfo.TryGetCharacteristicAndDifficulty(SongCore::API::Characteristics::SerializedName(characteristic), difficulty);
    if (!difficultyDataOpt.has_value()) return PostInstallBindings(GameplayCoreInstaller_InstallBindings, self);
    // get if one saber set
    auto& difficultyData = difficultyDataOpt->get();
    if (!difficultyData.oneSaber.has_value()) return PostInstallBindings(GameplayCoreInstaller_InstallBindings, self);

    int saberCount = difficultyData.oneSaber.value() ? 1 : 2;
    int origCount = characteristic->_numberOfColors;

    // override the color number during the invoke of orig, restore afterwards
    characteristic->_numberOfColors = saberCount;
    PostInstallBindings(GameplayCoreInstaller_InstallBindings, self);
    characteristic->_numberOfColors = origCount;
}

void NegativeNJSPatch(Zenject::DiContainer* container, GlobalNamespace::GameplayCoreInstaller* installer) {
    auto beatmapKey = installer->_sceneSetupData->beatmapKey;
    auto beatmapLevel = installer->_sceneSetupData->beatmapLevel;
    auto difficultyBeatmapData = beatmapLevel->GetDifficultyBeatmapData(beatmapKey.characteristic, beatmapKey.difficulty);
    auto noteJumpMovementSpeed = difficultyBeatmapData->noteJumpMovementSpeed;
    auto data = container->Resolve<GlobalNamespace::BeatmapObjectSpawnController::InitData*>();

    if(noteJumpMovementSpeed <= -GlobalNamespace::VariableMovementDataProvider::kMinNoteJumpMovementSpeed)
    {
        data->noteJumpMovementSpeed = noteJumpMovementSpeed;
    }
}

void PostInstallBindings(void(*GameplayCoreInstaller_InstallBindings)(GlobalNamespace::GameplayCoreInstaller* self), GlobalNamespace::GameplayCoreInstaller* instance) {

    GameplayCoreInstaller_InstallBindings(instance);
    NegativeNJSPatch(instance->Container, instance);

    // https://github.com/Kylemc1413/SongCore/blob/cd026d48171bb7fdee1a9d9646970b134f55228d/source/SongCore/Hooks/BindBeatmapLevelHook.cs#L24
    instance->Container->Bind<GlobalNamespace::BeatmapLevel*>()->FromInstance(instance->_sceneSetupData->beatmapLevel)->AsSingle();
}

