#include "include/UI/DeleteLevelButton.hpp"
#include "logging.hpp"

#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/Transform.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/ContentSizeFitter.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/BSML/Components/ButtonIconImage.hpp"

#include <chrono>
#include <future>

#define protected public
#include "bsml/shared/BSML/Tags/ButtonWithIconTag.hpp"
#undef protected

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

        // _deleteButtonRoot = BSML::ButtonWithIconTag().CreateObject(parent);
        // _deleteButton = _deleteButtonRoot->GetComponent<UnityEngine::UI::Button*>();
        // auto img = _deleteButtonRoot->GetComponent<BSML::ButtonIconImage*>();
        // img->SetIcon(_iconCache->DeleteIcon);

        auto _deleteButton = BSML::Lite::CreateUIButton(parent, "", std::bind(&DeleteLevelButton::AttemptDeleteCurrentlySelectedLevel, this));
        _deleteButtonRoot = _deleteButton->gameObject;
        _deleteButton->transform->localScale = detailView->practiceButton->transform->localScale;
        _deleteButton->transform->SetAsFirstSibling();
        auto content = _deleteButton->transform->Find("Content");
        UnityEngine::Object::Destroy(content->GetComponent<UnityEngine::UI::LayoutElement*>());
        UnityEngine::Object::Destroy(content->Find("Text")->gameObject);
        auto icon = BSML::Lite::CreateImage(content, _iconCache->DeleteIcon);
        icon->preserveAspect = true;

        // TODO: fix the button from being wide for some reason
        auto layout = _deleteButton->gameObject->GetComponent<UnityEngine::UI::LayoutElement*>();
        layout->preferredHeight = 10.0f;
        layout->preferredWidth = 10.0f;
        auto fitter = _deleteButton->gameObject->GetComponent<UnityEngine::UI::ContentSizeFitter*>();
        fitter->set_verticalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
        fitter->set_horizontalFit(UnityEngine::UI::ContentSizeFitter::FitMode::PreferredSize);
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
