#include "include/UI/DeleteLevelButton.hpp"
#include "logging.hpp"

#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/Transform.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML/Components/ButtonIconImage.hpp"

#include <chrono>
#include <future>

DEFINE_TYPE(SongCore::UI, DeleteLevelButton);

namespace SongCore::UI {
    void DeleteLevelButton::ctor(SongLoader::RuntimeSongLoader* runtimeSongLoader, GlobalNamespace::StandardLevelDetailViewController* standardLevelDetailViewController, IconCache* iconCache) {
        INVOKE_CTOR();
        _runtimeSongLoader = runtimeSongLoader;
        _levelDetailViewController = standardLevelDetailViewController;
        _iconCache = iconCache;
    }

    void DeleteLevelButton::Initialize() {
        _changeDifficultyBeatmapAction = BSML::MakeSystemAction<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::IDifficultyBeatmap*>(
            std::function<void(UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::IDifficultyBeatmap*)>(
                std::bind(&DeleteLevelButton::BeatmapLevelSelected, this, std::placeholders::_1, std::placeholders::_2)
            )
        );

        _levelDetailViewController->add_didChangeDifficultyBeatmapEvent(_changeDifficultyBeatmapAction);

        _changeContentAction = BSML::MakeSystemAction<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::StandardLevelDetailViewController::ContentType>(
            std::function<void(UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::StandardLevelDetailViewController::ContentType)>(
                std::bind(&DeleteLevelButton::LevelDetailContentChanged, this, std::placeholders::_1, std::placeholders::_2)
            )
        );

        _levelDetailViewController->add_didChangeContentEvent(_changeContentAction);

        auto detailView = _levelDetailViewController->_standardLevelDetailView;
        auto parent = detailView->practiceButton->transform->parent;

        // create button
        BSML::parse_and_construct("<icon-button id='_deleteButton' pref-width='10' pref-height='10'/>", parent, this);
        _deleteButtonRoot = _deleteButton->gameObject;
        /// make sure it's the leftmost button
        _deleteButtonRoot->transform->SetAsFirstSibling();
        // sprite, scale, skew fixing
        auto buttonImg = _deleteButtonRoot->GetComponent<BSML::ButtonIconImage*>();
        buttonImg->SetIcon(_iconCache->DeleteIcon);
        float scale = 1.7f;
        buttonImg->image->transform->localScale = {scale, scale, scale};
        auto imageView = il2cpp_utils::try_cast<HMUI::ImageView>(buttonImg->image).value_or(nullptr);
        if (imageView) {
            imageView->_skew = 0.18f;
        }
    }

    void DeleteLevelButton::Dispose() {
        _levelDetailViewController->remove_didChangeDifficultyBeatmapEvent(_changeDifficultyBeatmapAction);
        _levelDetailViewController->remove_didChangeContentEvent(_changeContentAction);
    }

    void DeleteLevelButton::Tick() {
        using namespace std::chrono_literals;
        if (_songDeleteFuture.valid() && _songDeleteFuture.wait_for(0ns) == std::future_status::ready) {
            _songDeleteFuture = std::future<void>();
            _runtimeSongLoader->RefreshSongs();
        }
    }

    void DeleteLevelButton::UpdateButtonState() {
        _deleteButtonRoot->SetActive(_lastSelectedCustomLevel != nullptr);
    }

    void DeleteLevelButton::AttemptDeleteCurrentlySelectedLevel() {
        DEBUG("Delete was pressed!");
    }

    void DeleteLevelButton::LevelDetailContentChanged(GlobalNamespace::StandardLevelDetailViewController* viewController, GlobalNamespace::StandardLevelDetailViewController::ContentType contentType) {
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

    void DeleteLevelButton::BeatmapLevelSelected(GlobalNamespace::StandardLevelDetailViewController* _, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap) {
        _lastSelectedDifficultyBeatmap = difficultyBeatmap;
        _lastSelectedCustomLevel = nullptr;
        if (!difficultyBeatmap) return;

        auto beatmapLevel = difficultyBeatmap->level;
        auto customBeatmapLevel = il2cpp_utils::try_cast<GlobalNamespace::CustomBeatmapLevel>(beatmapLevel).value_or(nullptr);

        if (customBeatmapLevel) {
            HandleCustomLevelWasSelected(customBeatmapLevel, difficultyBeatmap);
        } else { // not a custom level
            HandleVanillaLevelWasSelected(beatmapLevel, difficultyBeatmap);
        }
    }

    void DeleteLevelButton::HandleVanillaLevelWasSelected(GlobalNamespace::IBeatmapLevel* level, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap) {
        _lastSelectedDifficultyBeatmap = difficultyBeatmap;
        _lastSelectedCustomLevel = nullptr;
        UpdateButtonState();
    }

    void DeleteLevelButton::HandleCustomLevelWasSelected(GlobalNamespace::CustomBeatmapLevel* level, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap) {
        _lastSelectedDifficultyBeatmap = difficultyBeatmap;
        _lastSelectedCustomLevel = level;
        UpdateButtonState();
    }
}
