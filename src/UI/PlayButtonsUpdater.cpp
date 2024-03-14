#include "UI/PlayButtonsUpdater.hpp"
#include "CustomJSONData.hpp"
#include "logging.hpp"

#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"

#include "bsml/shared/Helpers/delegates.hpp"
#include <string_view>

DEFINE_TYPE(SongCore::UI, PlayButtonsUpdater);

namespace SongCore::UI {
    void PlayButtonsUpdater::ctor(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController,  PlayButtonInteractable* playButtonInteractable, Capabilities* capabilities) {
        _levelDetailViewController = levelDetailViewController;
        _playButtonInteractable;
        _capabilities = capabilities;
    }

    void PlayButtonsUpdater::Initialize() {
        _playButton = _levelDetailViewController->_standardLevelDetailView->actionButton;
        _practiceButton = _levelDetailViewController->_standardLevelDetailView->practiceButton;
        _anyDisablingModInfos = !_playButtonInteractable->PlayButtonDisablingModInfos.empty();

        _changeDifficultyBeatmapAction = BSML::MakeSystemAction<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::IDifficultyBeatmap*>(
            std::function<void(UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::IDifficultyBeatmap*)>(
                std::bind(&PlayButtonsUpdater::BeatmapLevelSelected, this, std::placeholders::_1, std::placeholders::_2)
            )
        );

        _levelDetailViewController->add_didChangeDifficultyBeatmapEvent(_changeDifficultyBeatmapAction);

        _changeContentAction = BSML::MakeSystemAction<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::StandardLevelDetailViewController::ContentType>(
            std::function<void(UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::StandardLevelDetailViewController::ContentType)>(
                std::bind(&PlayButtonsUpdater::LevelDetailContentChanged, this, std::placeholders::_1, std::placeholders::_2)
            )
        );

        _levelDetailViewController->add_didChangeContentEvent(_changeContentAction);
    }

    void PlayButtonsUpdater::Dispose() {
        _levelDetailViewController->remove_didChangeDifficultyBeatmapEvent(_changeDifficultyBeatmapAction);
        _levelDetailViewController->remove_didChangeContentEvent(_changeContentAction);
    }

    std::u16string_view DiffToStringView(GlobalNamespace::BeatmapDifficulty diff) {
        switch (diff) {
            case GlobalNamespace::BeatmapDifficulty::Easy:
                return u"Easy";
            case GlobalNamespace::BeatmapDifficulty::Normal:
                return u"Normal";
            case GlobalNamespace::BeatmapDifficulty::Hard:
                return u"Hard";
            case GlobalNamespace::BeatmapDifficulty::Expert:
                return u"Expert";
            case GlobalNamespace::BeatmapDifficulty::ExpertPlus:
                return u"ExpertPlus";
        }
        return u"Unknown";
    }

    bool PlayButtonsUpdater::IsPlayerAllowedToStart() {
        if (_anyDisablingModInfos) return false;
        if (!_lastSelectedDifficultyBeatmap) return false;
        if (!_lastSelectedCustomLevel) return true;

        // TODO: get requirements from the selected difficulty beatmap
        // use diff and characteristic to get the correct customData from the saveData & read requirements from there
        auto saveData = il2cpp_utils::cast<CustomJSONData::CustomLevelInfoSaveData>(_lastSelectedCustomLevel->standardLevelInfoSaveData);
        auto diff = _lastSelectedDifficultyBeatmap->get_difficulty();
        auto characteristic = _lastSelectedDifficultyBeatmap->parentDifficultyBeatmapSet->get_beatmapCharacteristic();

        auto levelDetailsOpt = saveData->TryGetBasicLevelDetails();
        if (!levelDetailsOpt.has_value()) return true;

        auto& levelDetails = levelDetailsOpt->get();
        auto difficultyBeatmapDetailsOpt = levelDetails.TryGetCharacteristicAndDifficulty(characteristic->serializedName, diff);
        if (!difficultyBeatmapDetailsOpt.has_value()) return true;

        auto& difficultyBeatmapDetails = difficultyBeatmapDetailsOpt->get();

        for (auto& requirement : difficultyBeatmapDetails.requirements) {
            if (!_capabilities->IsCapabilityRegistered(requirement)) return false;
        }

        return true;
    }

    void PlayButtonsUpdater::UpdatePlayButtonsState() {
        auto interactable = IsPlayerAllowedToStart();
        auto isCustom = _lastSelectedCustomLevel != nullptr;
        auto isWip = isCustom && _lastSelectedCustomLevel->levelID.ends_with(u" WIP");

        // if we allow player to start at all, practice button is enabled
        _practiceButton->set_interactable(interactable);
        // we want it to be "allow to start" and "not wip"
        _playButton->set_interactable(interactable && !isWip);
    }

    void PlayButtonsUpdater::LevelDetailContentChanged(GlobalNamespace::StandardLevelDetailViewController* viewController, GlobalNamespace::StandardLevelDetailViewController::ContentType contentType) {
        if (contentType == GlobalNamespace::StandardLevelDetailViewController::ContentType::OwnedAndReady) {
            auto difficultyBeatmap = viewController->selectedDifficultyBeatmap;
            _lastSelectedDifficultyBeatmap = difficultyBeatmap;
            _lastSelectedCustomLevel = nullptr;
            if (!difficultyBeatmap) return;

            auto beatmapLevel = difficultyBeatmap->level;
            auto customBeatmapLevel = il2cpp_utils::try_cast<GlobalNamespace::CustomBeatmapLevel>(beatmapLevel).value_or(nullptr);

            if (customBeatmapLevel) { // custom level
                HandleCustomLevelWasSelected(customBeatmapLevel, difficultyBeatmap);
            } else { // vanilla level
                HandleVanillaLevelWasSelected(beatmapLevel, difficultyBeatmap);
            }
        }
    }

    void PlayButtonsUpdater::BeatmapLevelSelected(GlobalNamespace::StandardLevelDetailViewController* _, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap) {
        DEBUG("Something was selected!");
        _lastSelectedDifficultyBeatmap = difficultyBeatmap;
        _lastSelectedCustomLevel = nullptr;
        if (!difficultyBeatmap) return;

        auto beatmapLevel = difficultyBeatmap->level;
        auto customBeatmapLevel = il2cpp_utils::try_cast<GlobalNamespace::CustomBeatmapLevel>(beatmapLevel).value_or(nullptr);
        DEBUG("LevelID {} was selected", difficultyBeatmap->level->i___GlobalNamespace__IPreviewBeatmapLevel()->get_levelID());

        if (customBeatmapLevel) {
            HandleCustomLevelWasSelected(customBeatmapLevel, difficultyBeatmap);
        } else { // not a custom level
            HandleVanillaLevelWasSelected(beatmapLevel, difficultyBeatmap);
        }
    }

    void PlayButtonsUpdater::HandleVanillaLevelWasSelected(GlobalNamespace::IBeatmapLevel* level, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap) {
        _lastSelectedDifficultyBeatmap = difficultyBeatmap;
        _lastSelectedCustomLevel = nullptr;
        UpdatePlayButtonsState();
    }

    void PlayButtonsUpdater::HandleCustomLevelWasSelected(GlobalNamespace::CustomBeatmapLevel* level, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap) {
        _lastSelectedDifficultyBeatmap = difficultyBeatmap;
        _lastSelectedCustomLevel = level;
        UpdatePlayButtonsState();
    }

    void PlayButtonsUpdater::HandleDisablingModInfosChanged(std::span<PlayButtonInteractable::PlayButtonDisablingModInfo const> disablingModInfos) {
        _anyDisablingModInfos = !disablingModInfos.empty();

        UpdatePlayButtonsState();
    }
}
