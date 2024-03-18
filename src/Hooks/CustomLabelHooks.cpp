#include "hooking.hpp"
#include "logging.hpp"

#include "UI/IconCache.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicCollection.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSegmentedControlController.hpp"
#include "GlobalNamespace/BeatmapDifficultySegmentedControlController.hpp"
#include "GlobalNamespace/BeatLineManager.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmap.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapDifficultyMethods.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "HMUI/IconSegmentedControl.hpp"
#include "HMUI/TextSegmentedControl.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Collections/Generic/IReadOnlyCollection_1.hpp"
#include "System/Collections/Generic/IEnumerable_1.hpp"
#include "System/Collections/Generic/IEnumerator_1.hpp"
#include "System/Collections/IEnumerable.hpp"
#include "System/Collections/IEnumerator.hpp"
#include "System/IDisposable.hpp"
#include "CustomJSONData.hpp"

struct TemporaryCharacteristicSegmentedControlData {
    int sortingOrder;
    UnityEngine::Sprite* icon;
    std::string label;
};

// hook which implements characteristic display on the level selection screena
MAKE_AUTO_HOOK_MATCH(BeatmapCharacteristicSegmentedControlController_SetData, &GlobalNamespace::BeatmapCharacteristicSegmentedControlController::SetData, void, GlobalNamespace::BeatmapCharacteristicSegmentedControlController* self, System::Collections::Generic::IReadOnlyList_1<::GlobalNamespace::IDifficultyBeatmapSet*>* difficultyBeatmapSets, GlobalNamespace::BeatmapCharacteristicSO* selectedBeatmapCharacteristic) {
    BeatmapCharacteristicSegmentedControlController_SetData(self, difficultyBeatmapSets, selectedBeatmapCharacteristic);

    // if these are null we can't do anything, just pretend it's not custom
    if (!difficultyBeatmapSets) return;

    auto count = il2cpp_utils::RunMethodRethrow<int>(difficultyBeatmapSets, "get_Count");

    ListW<UnityW<GlobalNamespace::BeatmapCharacteristicSO>> selfBeatmapCharacteristics(self->_beatmapCharacteristics);
    std::vector<TemporaryCharacteristicSegmentedControlData> cellData;

    int selectedCellIdx = self->_segmentedControl->selectedCellNumber;
    auto success = count != 0;
    for (int idx = 0; idx < count; idx++) {
        auto difficultyBeatmapSet = il2cpp_utils::RunMethodRethrow<GlobalNamespace::IDifficultyBeatmapSet*, false>(difficultyBeatmapSets, "get_Item", idx);
        auto characteristic = difficultyBeatmapSet->beatmapCharacteristic;

        auto customDifficultyBeatmapSet = il2cpp_utils::try_cast<GlobalNamespace::CustomDifficultyBeatmapSet>(difficultyBeatmapSet).value_or(nullptr);
        if (!customDifficultyBeatmapSet) { success = false; break; }

        auto firstDiffBeatmap = customDifficultyBeatmapSet->difficultyBeatmaps->get_Item(0);
        auto customLevel = il2cpp_utils::try_cast<GlobalNamespace::CustomBeatmapLevel>(firstDiffBeatmap->level).value_or(nullptr);
        if (!customLevel) { success = false; break; }

        auto saveData = il2cpp_utils::try_cast<SongCore::CustomJSONData::CustomLevelInfoSaveData>(customLevel->standardLevelInfoSaveData).value_or(nullptr);
        if (!saveData) { success = false; break; }
        auto characteristicDetailsOpt = saveData->TryGetCharacteristic(characteristic->serializedName);
        if (!characteristicDetailsOpt.has_value()) { success = false; break; }
        auto& characteristicDetails = characteristicDetailsOpt->get();

        auto label = characteristicDetails.characteristicLabel.value_or(characteristicDetails.characteristicName);
        UnityEngine::Sprite* icon = nullptr;
        if (characteristicDetails.characteristicIconImageFileName.has_value() && !characteristicDetails.characteristicIconImageFileName->empty()) {
            auto iconCache = SongCore::UI::IconCache::_instance;
            if (iconCache) { // if iconcache instance exists, use it
                auto iconPath = std::filesystem::path(customLevel->get_customLevelPath()) / characteristicDetails.characteristicIconImageFileName.value();
                icon = iconCache->GetIconForPath(iconPath);
            }
        }

        cellData.emplace_back(
            characteristic->sortingOrder,
            icon ? icon : characteristic->icon.unsafePtr(),
            label
        );
    }

    if (success) {
        std::stable_sort(cellData.begin(), cellData.end(), [](auto& a, auto& b){ return a.sortingOrder < b.sortingOrder; });
        ArrayW<HMUI::IconSegmentedControl::DataItem*> dataItems(cellData.size());
        for (int i = 0; auto& [_, icon, label] : cellData) {
            dataItems[i++] = HMUI::IconSegmentedControl::DataItem::New_ctor(
                icon, label
            );
        }

        self->_segmentedControl->SetData(dataItems);
        self->_segmentedControl->SelectCellWithNumber(selectedCellIdx);
    }
}

// hook which implements custom difficulty names on the level selection screen
MAKE_AUTO_HOOK_MATCH(BeatmapDifficultySegmentedControlController_SetData, &GlobalNamespace::BeatmapDifficultySegmentedControlController::SetData, void, GlobalNamespace::BeatmapDifficultySegmentedControlController* self, System::Collections::Generic::IReadOnlyList_1<GlobalNamespace::IDifficultyBeatmap*>* difficultyBeatmapsList, GlobalNamespace::BeatmapDifficulty selectedDifficulty) {
    BeatmapDifficultySegmentedControlController_SetData(self, difficultyBeatmapsList, selectedDifficulty);

    // if these are null we can't do anything, just pretend it's not custom
    if (!difficultyBeatmapsList) return;

    auto labels = ListW<StringW>::New();

    auto count = il2cpp_utils::RunMethodRethrow<int>(difficultyBeatmapsList, "get_Count");

    auto success = count != 0;
    for (int idx = 0; idx < count; idx++) {
        auto difficultyBeatmap = il2cpp_utils::RunMethodRethrow<GlobalNamespace::IDifficultyBeatmap*, false>(difficultyBeatmapsList, "get_Item", idx);
        auto customLevel = il2cpp_utils::try_cast<GlobalNamespace::CustomBeatmapLevel>(difficultyBeatmap->level).value_or(nullptr);
        if (!customLevel) { success = false; break; }

        auto saveData = il2cpp_utils::try_cast<SongCore::CustomJSONData::CustomLevelInfoSaveData>(customLevel->standardLevelInfoSaveData).value_or(nullptr);
        if (!saveData) { success = false; break; }

        auto characteristic = difficultyBeatmap->parentDifficultyBeatmapSet->beatmapCharacteristic;
        auto difficulty = difficultyBeatmap->difficulty;
        auto difficultyDetailsOpt = saveData->TryGetCharacteristicAndDifficulty(characteristic->serializedName, difficulty);
        if (!difficultyDetailsOpt.has_value()) { success = false; break; }

        auto& difficultyDetails = difficultyDetailsOpt->get();
        if (difficultyDetails.customDiffLabel.has_value() && !difficultyDetails.customDiffLabel->empty()) {
            labels->Add(difficultyDetails.customDiffLabel.value());
        } else {
            labels->Add(GlobalNamespace::BeatmapDifficultyMethods::Name(difficulty));
        }
    }

    if (success) { // we found labels
        self->_difficultySegmentedControl->SetTexts(labels->i___System__Collections__Generic__IReadOnlyList_1_T_());
    }
}
