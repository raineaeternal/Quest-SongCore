#include "UI/ProgressBar.hpp"
#include "SongLoader/RuntimeSongLoader.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "logging.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "bsml/shared/BSML/SharedCoroutineStarter.hpp"
#include <string>
#include "UnityEngine/Time.hpp"

DEFINE_TYPE(SongCore::UI, ProgressBar);

namespace SongCore::UI {
    void ProgressBar::ctor() {
        _pos = UnityEngine::Vector3(0, 0.05f, 3);
        _rot = UnityEngine::Vector3(90, 0, 0);
        _scale = UnityEngine::Vector3(0.1f, 0.1f, 0.0f);

        _canvasScale = UnityEngine::Vector2(100, 50);
        _authorNamePos = UnityEngine::Vector2(10, 31);
        _headerPos = UnityEngine::Vector2(10, 15);
        _headerSize = UnityEngine::Vector2(100, 20);
        
        std::string HeaderText = "Loading songs...";
        std::string PluginText = "SongCore Loader";
        
        float _headerTextSize = 15.0f;
        float _pluginTextSize = 9.0f;

        _pluginTextPos = UnityEngine::Vector2(10, 23);

        _loadingBarSize = UnityEngine::Vector2(10, 1);
        _bgColor = UnityEngine::Color(0, 0, 0, 0.2f);
    }

    void ProgressBar::Inject(SongLoader::RuntimeSongLoader* runtimeSongLoader) {
        DEBUG("ProgressBar::Inject");
        _runtimeSongLoader = runtimeSongLoader;
    }

    void ProgressBar::Initialize() {
        INFO("Connected RSL instance: {}", fmt::ptr(_runtimeSongLoader));

        _runtimeSongLoader->SongsWillRefresh += {&ProgressBar::RuntimeSongLoaderOnSongRefresh, this};
        _runtimeSongLoader->SongsLoaded += {&ProgressBar::RuntimeSongLoaderOnSongLoaded, this};

        BSML::SharedCoroutineStarter::StartCoroutine(custom_types::Helpers::CoroutineHelper::New([this]() -> custom_types::Helpers::Coroutine {
            co_yield nullptr;
            auto gameObject = UnityEngine::GameObject::New_ctor();
            gameObject->transform->position = _pos;
            gameObject->transform->eulerAngles = _rot;
            gameObject->transform->localScale = _scale;

            _canvas = gameObject->AddComponent<UnityEngine::Canvas *>();
            _canvas->renderMode = UnityEngine::RenderMode::WorldSpace;
            _canvas->enabled = true;

            auto rect = _canvas->transform.cast<UnityEngine::RectTransform>();
            rect->sizeDelta = _canvasScale;

            _pluginNameText = BSML::Lite::CreateText(_canvas->transform.cast<UnityEngine::RectTransform>(), PluginText, _pluginTextSize, _pluginTextPos);
            rect = _pluginNameText->transform.cast<UnityEngine::RectTransform>();
            rect->SetParent(_canvas->transform, false);
            rect->anchoredPosition = _pluginTextPos;
            _pluginNameText->text = PluginText;
            _pluginNameText->fontSize = _pluginTextSize;

            _headerText = BSML::Lite::CreateText(_canvas->transform.cast<UnityEngine::RectTransform>(), HeaderText, _headerPos);
            rect = _headerText->transform.cast<UnityEngine::RectTransform>();
            rect->SetParent(_canvas->transform, false);
            rect->anchoredPosition = _headerPos;
            rect->sizeDelta = _headerSize;
            _headerText->text = HeaderText;
            _headerText->fontSize = _headerTextSize;

            _loadingBg = UnityEngine::GameObject::New_ctor("Background")->AddComponent<UnityEngine::UI::Image *>();
            rect = _loadingBg->transform.cast<UnityEngine::RectTransform>();
            rect->SetParent(_canvas->transform, false);
            rect->sizeDelta = _loadingBarSize;
            _loadingBg->color = _bgColor;

            _loadingBar = UnityEngine::GameObject::New_ctor("Loading Bar")->AddComponent<UnityEngine::UI::Image *>();
            rect = _loadingBar->transform.cast<UnityEngine::RectTransform>();
            rect->SetParent(_canvas->transform, false);
            rect->sizeDelta = _loadingBarSize;
            auto tex = UnityEngine::Texture2D::get_whiteTexture();
            auto sprite = UnityEngine::Sprite::Create(tex, UnityEngine::Rect(0, 0, tex->width, tex->height), {1, 1}, 100, 1);
            _loadingBar->sprite = sprite;
            _loadingBar->type = UnityEngine::UI::Image::Type::Filled;
            _loadingBar->fillMethod = UnityEngine::UI::Image::FillMethod::Horizontal;
            _loadingBar->color = UnityEngine::Color(1, 1, 1, 0.5f);

            co_return;
        }()));
    }

    void ProgressBar::ShowMessage(std::string message) {
        BSML::SharedCoroutineStarter::StopCoroutine(_coroReturn);
        _showingMessage = true;
        _headerText->text = message;
        _loadingBar->enabled = false;
        _loadingBg->enabled = false;
        _canvas->enabled = true;
    }

    void ProgressBar::ShowMessage(std::string message, float time) {
        _showingMessage = true;
        _headerText->text = message;
        _loadingBar->enabled = false;
        _loadingBg->enabled = false;
        _canvas->enabled = true;
    }

    custom_types::Helpers::Coroutine ProgressBar::DisableCanvasRoutine(float time) {
        while (time > 0) {
            time -= UnityEngine::Time::get_deltaTime();
            co_yield nullptr;

            co_return;
        }
        _canvas->enabled = false;
        _showingMessage = false;
    }

    void ProgressBar::RuntimeSongLoaderOnSongRefresh() {
        BSML::SharedCoroutineStarter::StopCoroutine(_coroReturn);
        _showingMessage = false;
        _headerText->text = HeaderText;
        _loadingBar->enabled = true;
        _loadingBg->enabled = true;
        _canvas->enabled = true;
    }

    void ProgressBar::RuntimeSongLoaderOnSongLoaded(std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> customLevels) {
        _showingMessage = false;
        std::string songOrSongs = customLevels.size() == 1 ? "song" : "songs";
        _headerText->text = fmt::format("{} {} loaded", customLevels.size(), songOrSongs);
        _loadingBar->enabled = false;
        _loadingBg->enabled = false;
        _coroReturn = BSML::SharedCoroutineStarter::StartCoroutine(DisableCanvasRoutine(5));
    }

    void ProgressBar::Tick() {
        if (!_canvas || !_canvas->enabled) return;

        _loadingBar->fillAmount = _runtimeSongLoader->Progress;
    }

    void ProgressBar::Dispose() {
        _runtimeSongLoader->SongsWillRefresh -= {&ProgressBar::RuntimeSongLoaderOnSongRefresh, this};
        _runtimeSongLoader->SongsLoaded -= {&ProgressBar::RuntimeSongLoaderOnSongLoaded, this};
    }
}
