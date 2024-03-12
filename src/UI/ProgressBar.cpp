#include "UI/ProgressBar.hpp"
#include "SongLoader/RuntimeSongLoader.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "UnityEngine/Texture2D.hpp"
#include "logging.hpp"
#include "custom-types/shared/coroutine.hpp"

DEFINE_TYPE(SongCore::UI, ProgressBar);

namespace SongCore::UI {
    void ProgressBar::ctor() {
        _pos = UnityEngine::Vector3(0, 2.5f, 2.5f);
        _rot = UnityEngine::Vector3(0, 0, 0);
        _scale = UnityEngine::Vector3(0.1f, 0.1f, 0.1f);

        _canvasScale = UnityEngine::Vector2(100, 50);
        _authorNamePos = UnityEngine::Vector2(10, 31);
        _headerPos = UnityEngine::Vector2(10, 15);
        _headerSize = UnityEngine::Vector2(100, 20);

        _pluginTextPos = UnityEngine::Vector2(10, 23);

        _loadingBarSize = UnityEngine::Vector2(100, 10);
        _bgColor = UnityEngine::Color(0, 0, 0, 0.2f);
    }

    void ProgressBar::Inject(SongLoader::RuntimeSongLoader* runtimeSongLoader) {
        DEBUG("ProgressBar::Inject");
        _runtimeSongLoader = runtimeSongLoader;
    }

    void ProgressBar::Awake() {
        INFO("Connected RSL instance: {}", fmt::ptr(_runtimeSongLoader));
        StartCoroutine(custom_types::Helpers::CoroutineHelper::New([this]() -> custom_types::Helpers::Coroutine {
            co_yield nullptr;
            gameObject->transform->position = _pos;
            gameObject->transform->eulerAngles = _rot;
            gameObject->transform->localScale = _scale;

            _canvas = gameObject->AddComponent<UnityEngine::Canvas *>();
            _canvas->renderMode = UnityEngine::RenderMode::WorldSpace;
            _canvas->enabled = true;

            auto rect = _canvas->transform.cast<UnityEngine::RectTransform>();
            rect->sizeDelta = _canvasScale;

            _pluginNameText = BSML::Lite::CreateText(_canvas->transform.cast<UnityEngine::RectTransform>(), PluginText, pluginTextSize, _pluginTextPos);
            rect = _pluginNameText->transform.cast<UnityEngine::RectTransform>();
            rect->SetParent(_canvas->transform, false);
            rect->anchoredPosition = _pluginTextPos;
            _pluginNameText->text = PluginText;
            _pluginNameText->fontSize = pluginTextSize;

            _headerText = BSML::Lite::CreateText(_canvas->transform.cast<UnityEngine::RectTransform>(), HeaderText, _headerPos);
            rect = _headerText->transform.cast<UnityEngine::RectTransform>();
            rect->SetParent(_canvas->transform, false);
            rect->anchoredPosition = _headerPos;
            rect->sizeDelta = _headerSize;
            _headerText->text = HeaderText;
            _headerText->fontSize = headerTextSize;

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

    void ProgressBar::Update() {
        if (!_canvas || !_canvas->enabled) return;
        INFO("Current progress: {}", _runtimeSongLoader->Progress);

        _loadingBar->fillAmount = _runtimeSongLoader->Progress;
    }
}
