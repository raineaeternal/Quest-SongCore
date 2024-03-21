#include "UI/PlayButtonsUpdater.hpp"
#include "CustomJSONData.hpp"
#include "LevelSelect.hpp"
#include "logging.hpp"

#include "bsml/shared/Helpers/delegates.hpp"
#include <string_view>

DEFINE_TYPE(SongCore::UI, PlayButtonsUpdater);

namespace SongCore::UI {
    void PlayButtonsUpdater::ctor(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, PlayButtonInteractable* playButtonInteractable, Capabilities* capabilities, LevelSelect* levelSelect) {
        _levelDetailViewController = levelDetailViewController;
        _playButtonInteractable = playButtonInteractable;
        _capabilities = capabilities;
        _levelSelect = levelSelect;
    }

    void PlayButtonsUpdater::Initialize() {
        _playButton = _levelDetailViewController->_standardLevelDetailView->actionButton;
        _practiceButton = _levelDetailViewController->_standardLevelDetailView->practiceButton;
        _anyDisablingModInfos = !_playButtonInteractable->PlayButtonDisablingModInfos.empty();

        _playButtonInteractable->PlayButtonDisablingModsChanged += {&PlayButtonsUpdater::HandleDisablingModInfosChanged};
        _levelSelect->LevelWasSelected += {&PlayButtonsUpdater::LevelWasSelected, this};

        HandleDisablingModInfosChanged(_playButtonInteractable->PlayButtonDisablingModInfos);
    }

    void PlayButtonsUpdater::Dispose() {
        _playButtonInteractable->PlayButtonDisablingModsChanged -= {&PlayButtonsUpdater::HandleDisablingModInfosChanged};
        _levelSelect->LevelWasSelected -= {&PlayButtonsUpdater::LevelWasSelected, this};
    }

    bool PlayButtonsUpdater::IsPlayerAllowedToStart() {
        if (_anyDisablingModInfos) return false;
        if (!_levelIsCustom) return true;
        if (_missingRequirements) return false;

        return true;
    }

    void PlayButtonsUpdater::UpdatePlayButtonsState() {
        auto interactable = IsPlayerAllowedToStart();

        // if we allow player to start at all, practice button is enabled
        _practiceButton->set_interactable(interactable);
        // we want it to be "allow to start" and "not wip"
        _playButton->set_interactable(interactable && !_levelIsWIP);
    }

    void PlayButtonsUpdater::LevelWasSelected(LevelSelect::LevelWasSelectedEventArgs const& eventArgs) {
        _levelIsCustom = eventArgs.isCustom;
        _levelIsWIP = eventArgs.isWIP;

        _missingRequirements = false;
        if (eventArgs.customLevelDetails.has_value()) {
            for (auto& requirement : eventArgs.customLevelDetails->difficultyDetails.requirements) {
                if (!_capabilities->IsCapabilityRegistered(requirement)) {
                    _missingRequirements = true;
                    break;
                }
            }
        }

        UpdatePlayButtonsState();
    }

    void PlayButtonsUpdater::HandleDisablingModInfosChanged(std::span<PlayButtonInteractable::PlayButtonDisablingModInfo const> disablingModInfos) {
        _anyDisablingModInfos = !disablingModInfos.empty();

        UpdatePlayButtonsState();
    }
}
