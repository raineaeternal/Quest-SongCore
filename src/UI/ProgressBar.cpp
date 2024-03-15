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
        _scale = UnityEngine::Vector3(0.02f, 0.02f, 0.0f);

        _canvasScale = UnityEngine::Vector2(100, 50);
        _authorNamePos = UnityEngine::Vector2(10, 31);
        _headerPos = UnityEngine::Vector2(10, 15);
        _headerSize = UnityEngine::Vector2(100, 20);

        HeaderText = "Loading songs...";
        PluginText = "SongCore Loader";

        _headerTextSize = 15.0f;
        _pluginTextSize = 9.0f;

        _pluginTextPos = UnityEngine::Vector2(10, 23);

        _loadingBarSize = UnityEngine::Vector2(100, 10);
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

        auto gameObject = UnityEngine::GameObject::New_ctor();
        UnityEngine::Object::DontDestroyOnLoad(gameObject);

        gameObject->transform->position = _pos;
        gameObject->transform->eulerAngles = _rot;
        gameObject->transform->localScale = _scale;

        _canvas = gameObject->AddComponent<UnityEngine::Canvas *>();
        _canvasGroup = gameObject->AddComponent<UnityEngine::CanvasGroup*>();
        _canvasGroup->alpha = 1.0f;
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

        if (_runtimeSongLoader->AreSongsLoaded) {
            RuntimeSongLoaderOnSongLoaded(_runtimeSongLoader->AllLevels);
        }
    }

    void ProgressBar::ShowMessage(std::string message) {
        _showingMessage = true;
        _headerText->text = message;
        _loadingBar->enabled = false;
        _loadingBg->enabled = false;
        _canvas->enabled = true;
        ShowCanvasForSeconds(5);
    }

    void ProgressBar::ShowMessage(std::string message, float time) {
        _showingMessage = true;
        _headerText->text = message;
        _loadingBar->enabled = false;
        _loadingBg->enabled = false;
        _canvas->enabled = true;
        ShowCanvasForSeconds(5);
    }

    custom_types::Helpers::Coroutine ProgressBar::DisableCanvasRoutine(float time) {
        float totalTime = time;
        while (time > 0) {
            auto deltaTime = UnityEngine::Time::get_deltaTime();
            time -= deltaTime;
            co_yield nullptr;
            float alpha = _canvasGroup->alpha;
            if ((totalTime - time) < 0.25 && alpha < 1.0f) {
                alpha += deltaTime * 4;
            }

            // towards the end, fade out the canvas
            if (time < 0.25f) {
                alpha = time * 4;
            }

            _canvasGroup->alpha = alpha;
            if (time < 0.25f) {
                _canvasGroup->alpha = time * 4;
            }
        }
        _canvasGroup->alpha = 0.0f;
        _canvas->enabled = false;
        _showingMessage = false;

        co_return;
    }

    void ProgressBar::ShowCanvasForSeconds(float time) {
        StopDisableCanvasRoutine();
        _disableCanvasRoutine = BSML::SharedCoroutineStarter::StartCoroutine(DisableCanvasRoutine(5));
    }

    void ProgressBar::StopDisableCanvasRoutine() {
        if (_disableCanvasRoutine) BSML::SharedCoroutineStarter::StopCoroutine(_disableCanvasRoutine);
        _disableCanvasRoutine = nullptr;
    }

    void ProgressBar::RuntimeSongLoaderOnSongRefresh() {
        StopDisableCanvasRoutine();
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
        _loadingBar->fillAmount = 1.0f;
        ShowCanvasForSeconds(5);
    }

    void ProgressBar::Tick() {
        if (!_canvas || !_canvas->enabled) return;

        if (_runtimeSongLoader->AreSongsRefreshing) {
            _loadingBar->fillAmount = _runtimeSongLoader->Progress;
        }
    }

    void ProgressBar::Dispose() {
        StopDisableCanvasRoutine();
        if (_canvas && _canvas->m_CachedPtr) {
            UnityEngine::Object::Destroy(_canvas->gameObject);
        }
        _canvas = nullptr;

        _runtimeSongLoader->SongsWillRefresh -= {&ProgressBar::RuntimeSongLoaderOnSongRefresh, this};
        _runtimeSongLoader->SongsLoaded -= {&ProgressBar::RuntimeSongLoaderOnSongLoaded, this};
    }
}
