#include <GlobalNamespace/VariableMovementDataProvider.hpp>

#include "Characteristics.hpp"
#include "hooking.hpp"
#include "logging.hpp"
#include "config.hpp"

#include "CustomJSONData.hpp"
#include "GlobalNamespace/BeatmapBasicData.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnController.hpp"
#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "SongLoader/CustomBeatmapLevel.hpp"
#include "Zenject/DiContainer.hpp"
#include "Zenject/FromBinderGeneric_1.hpp"

void PostInstallBindings(void(*GameplayCoreInstaller_InstallBindings)(GlobalNamespace::GameplayCoreInstaller* self), GlobalNamespace::GameplayCoreInstaller* instance);

// GameplayCoreInstaller::InstallBindings
// POSTFIX
   // NegativeNJS patch
   // Fetches negative NJS from difficultyBeatmapData and updates to BeatmapObjectSpawnController::InitData
// NOTE: the one saber override used to live here, mutating characteristic->_numberOfColors. That field no longer
// exists (NumberOfColors is now a computed static BeatmapCharacteristicExtensions method), so it's handled instead
// by SongCore::Overrides::OneSaberOverride + the SaberManager::Start hook in src/Hooks/OneSaberHooks.cpp.
MAKE_AUTO_HOOK_MATCH(GameplayCoreInstaller_InstallBindings, &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void, GlobalNamespace::GameplayCoreInstaller* self) {
    PostInstallBindings(GameplayCoreInstaller_InstallBindings, self);
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

