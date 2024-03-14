#include "hooking.hpp"
#include "logging.hpp"
#include "config.hpp"

#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "CustomJSONData.hpp"

MAKE_AUTO_HOOK_MATCH(GameplayCoreInstaller_InstallBindings, &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void, GlobalNamespace::GameplayCoreInstaller* self) {
    // if override is disabledl, just run orig and nothing else
    if (config.disableOneSaberOverride) return GameplayCoreInstaller_InstallBindings(self);

    // get custom preview level
    auto level = il2cpp_utils::try_cast<GlobalNamespace::CustomPreviewBeatmapLevel>(self->_sceneSetupData->previewBeatmapLevel).value_or(nullptr);
    if (!level) return GameplayCoreInstaller_InstallBindings(self);

    // get custom level save data if possible
    auto saveData = il2cpp_utils::try_cast<SongCore::CustomJSONData::CustomLevelInfoSaveData>(level->standardLevelInfoSaveData).value_or(nullptr);
    if (!saveData) return GameplayCoreInstaller_InstallBindings(self);

    // get difficulty data
    auto difficultyBeatmap = self->_sceneSetupData->difficultyBeatmap;
    auto characteristic = difficultyBeatmap->parentDifficultyBeatmapSet->beatmapCharacteristic;
    auto difficulty = difficultyBeatmap->difficulty;
    auto difficultyDataOpt = saveData->TryGetCharacteristicAndDifficulty(characteristic->serializedName, difficulty);
    if (!difficultyDataOpt.has_value()) return GameplayCoreInstaller_InstallBindings(self);

    // get if one saber set
    auto& difficultyData = difficultyDataOpt->get();
    if (!difficultyData.oneSaber.has_value()) return GameplayCoreInstaller_InstallBindings(self);

    int saberCount = difficultyData.oneSaber.value() ? 1 : 2;
    int origCount = characteristic->_numberOfColors;

    // override the color number during the invoke of orig, restore afterwards
    characteristic->_numberOfColors = saberCount;
    GameplayCoreInstaller_InstallBindings(self);
    characteristic->_numberOfColors = origCount;
}
