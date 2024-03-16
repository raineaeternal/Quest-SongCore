#include "LevelSelect.hpp"
#include "logging.hpp"

#include "CustomJSONData.hpp"
#include "SongCore.hpp"

#include "SongLoader/RuntimeSongLoader.hpp"

#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "SongLoader/RuntimeSongLoader.hpp"
#include "bsml/shared/Helpers/delegates.hpp"

DEFINE_TYPE(SongCore, LevelSelect);

namespace SongCore {
    void LevelSelect::ctor(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController) {
        INVOKE_CTOR();
        _levelDetailViewController = levelDetailViewController;
    }

    void LevelSelect::Initialize() {
        _changeDifficultyBeatmapAction = BSML::MakeSystemAction<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::IDifficultyBeatmap*>(
            std::function<void(UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::IDifficultyBeatmap*)>(
                std::bind(&LevelSelect::BeatmapLevelSelected, this, std::placeholders::_1, std::placeholders::_2)
            )
        );

        _levelDetailViewController->add_didChangeDifficultyBeatmapEvent(_changeDifficultyBeatmapAction);

        _changeContentAction = BSML::MakeSystemAction<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::StandardLevelDetailViewController::ContentType>(
            std::function<void(UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::StandardLevelDetailViewController::ContentType)>(
                std::bind(&LevelSelect::LevelDetailContentChanged, this, std::placeholders::_1, std::placeholders::_2)
            )
        );

        _levelDetailViewController->add_didChangeContentEvent(_changeContentAction);
    }

    void LevelSelect::Dispose() {
        _levelDetailViewController->remove_didChangeDifficultyBeatmapEvent(_changeDifficultyBeatmapAction);
        _levelDetailViewController->remove_didChangeContentEvent(_changeContentAction);
    }

    void LevelSelect::LevelDetailContentChanged(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, GlobalNamespace::StandardLevelDetailViewController::ContentType contentType) {
        if (contentType == GlobalNamespace::StandardLevelDetailViewController::ContentType::OwnedAndReady) {
            StartLevelWasSelectedInvoke();
        }
    }

    void LevelSelect::BeatmapLevelSelected(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, GlobalNamespace::IDifficultyBeatmap* selectedBeatmap) {
        StartLevelWasSelectedInvoke();
    }

    void LevelSelect::StartLevelWasSelectedInvoke() {
        LevelWasSelectedEventArgs eventArgs;

        eventArgs.levelPack = GetSelectedLevelPack();
        eventArgs.previewBeatmapLevel = GetSelectedPreviewBeatmapLevel();
        eventArgs.beatmapLevel = GetSelectedBeatmapLevel();
        eventArgs.difficultyBeatmapSet = GetSelectedDifficultyBeatmapSet();
        eventArgs.difficultyBeatmap = GetSelectedDifficultyBeatmap();

        // if anything isn't set, early return because the selection performed was invalid
        if (!eventArgs.levelPack) return;
        if (!eventArgs.previewBeatmapLevel) return;
        if (!eventArgs.beatmapLevel) return;
        if (!eventArgs.difficultyBeatmapSet) return;
        if (!eventArgs.difficultyBeatmap) return;

        eventArgs.levelID = static_cast<std::string>(eventArgs.previewBeatmapLevel->levelID);

        auto customLevel = il2cpp_utils::try_cast<GlobalNamespace::CustomBeatmapLevel>(eventArgs.beatmapLevel).value_or(nullptr);
        if (customLevel) {
            eventArgs.isCustom = true;
            HandleCustomLevelWasSelected(eventArgs);
        }

        InvokeLevelWasSelected(eventArgs);
    }

    void LevelSelect::HandleCustomLevelWasSelected(LevelWasSelectedEventArgs& eventArgs) {
        if (!eventArgs.isCustom) return;

        eventArgs.isWIP = eventArgs.levelID.ends_with(" WIP");
        // get a view into the levelid
        std::string_view hashView(eventArgs.levelID);
        // remove `custom_level_` from the front
        hashView = hashView.substr(SongLoader::RuntimeSongLoader::CUSTOM_LEVEL_PREFIX_ID.size());
        // possibly remove ` WIP` from the end
        eventArgs.hash = hashView.substr(0, hashView.size() - (eventArgs.isWIP ? 4 : 0));

        auto saveData = il2cpp_utils::try_cast<CustomJSONData::CustomLevelInfoSaveData>(eventArgs.customBeatmapLevel->standardLevelInfoSaveData).value_or(nullptr);
        if (!saveData) return;

        eventArgs.customLevelInfoSaveData = saveData;

        auto levelDetails = saveData->TryGetBasicLevelDetails();
        if (!levelDetails.has_value()) return;

        auto characteristicDetails = levelDetails->get().TryGetCharacteristic(eventArgs.difficultyBeatmapSet->beatmapCharacteristic->serializedName);
        if (!characteristicDetails.has_value()) return;

        auto diffDetails = characteristicDetails->get().TryGetDifficulty(eventArgs.difficultyBeatmap->difficulty);
        if (!diffDetails.has_value()) return;

        eventArgs.customLevelDetails.emplace(
            levelDetails.value(),
            characteristicDetails.value(),
            diffDetails.value()
        );
    }

    void LevelSelect::InvokeLevelWasSelected(LevelWasSelectedEventArgs const& eventArgs) {
        LevelWasSelected.invoke(eventArgs);
        SongCore::API::LevelSelect::GetLevelWasSelectedEvent().invoke(eventArgs);
    }

    GlobalNamespace::IBeatmapLevelPack* LevelSelect::GetSelectedLevelPack() {
        return _levelDetailViewController->_pack;
    }

    GlobalNamespace::IPreviewBeatmapLevel* LevelSelect::GetSelectedPreviewBeatmapLevel() {
        return _levelDetailViewController->beatmapLevel;
    }

    GlobalNamespace::IBeatmapLevel* LevelSelect::GetSelectedBeatmapLevel() {
        return _levelDetailViewController->selectedDifficultyBeatmap ? _levelDetailViewController->selectedDifficultyBeatmap->level : nullptr;
    }

    GlobalNamespace::IDifficultyBeatmapSet* LevelSelect::GetSelectedDifficultyBeatmapSet() {
        return _levelDetailViewController->selectedDifficultyBeatmap ? _levelDetailViewController->selectedDifficultyBeatmap->parentDifficultyBeatmapSet : nullptr;
    }

    GlobalNamespace::IDifficultyBeatmap* LevelSelect::GetSelectedDifficultyBeatmap() {
        return _levelDetailViewController->selectedDifficultyBeatmap;
    }
}
