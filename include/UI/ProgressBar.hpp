#pragma once

#include "SongLoader/RuntimeSongLoader.hpp"
#include "custom-types/shared/macros.hpp"
#include "lapiz/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "Zenject/IInitializable.hpp"
#include "System/IDisposable.hpp"
#include "Zenject/ITickable.hpp"
#include "UnityEngine/Canvas.hpp"
#include "TMPro/TMP_Text.hpp"

#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/RenderMode.hpp"

#include "SongLoader/RuntimeSongLoader.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(SongCore::UI, ProgressBar, System::Object, std::vector<Il2CppClass *>({classof(::Zenject::IInitializable *), classof(::System::IDisposable *), classof(::Zenject::ITickable *)}),
    DECLARE_CTOR(ctor);
    DECLARE_INJECT_METHOD(void, Inject, SongLoader::RuntimeSongLoader* runtimeSongLoader);
    DECLARE_INSTANCE_FIELD(SongCore::SongLoader::RuntimeSongLoader*, _runtimeSongLoader);

    DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::Canvas*, _canvas);
    DECLARE_INSTANCE_FIELD_PRIVATE(TMPro::TMP_Text*, _authorNameText);
    DECLARE_INSTANCE_FIELD_PRIVATE(TMPro::TMP_Text*, _pluginNameText);
    DECLARE_INSTANCE_FIELD_PRIVATE(TMPro::TMP_Text*, _headerText);
    DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::UI::Image*, _loadingBg);
    DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::UI::Image*, _loadingBar);

    DECLARE_INSTANCE_FIELD(UnityEngine::Vector3, _pos);
    DECLARE_INSTANCE_FIELD(UnityEngine::Vector3, _rot);
    DECLARE_INSTANCE_FIELD(UnityEngine::Vector3, _scale);

    DECLARE_INSTANCE_FIELD(UnityEngine::Vector2, _canvasScale);
    DECLARE_INSTANCE_FIELD(UnityEngine::Vector2, _authorNamePos);
    DECLARE_INSTANCE_FIELD(UnityEngine::Vector2, _headerPos);
    DECLARE_INSTANCE_FIELD(UnityEngine::Vector2, _headerSize);

    DECLARE_INSTANCE_FIELD(UnityEngine::Vector2, _pluginTextPos);

    DECLARE_INSTANCE_FIELD(UnityEngine::Vector2, _loadingBarSize);
    DECLARE_INSTANCE_FIELD(UnityEngine::Color, _bgColor);

    UnityEngine::Coroutine* _coroReturn;

    std::string HeaderText;
    std::string PluginText;

    float _headerTextSize;
    float _pluginTextSize;

    bool _showingMessage;

    void ShowMessage(std::string message, float time);
    void ShowMessage(std::string message);

    void RuntimeSongLoaderOnSongRefresh();
    void RuntimeSongLoaderOnSongLoaded(std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> customLevels);
    custom_types::Helpers::Coroutine DisableCanvasRoutine(float time);
    void StopDisableCanvasRoutine();

    DECLARE_OVERRIDE_METHOD_MATCH(void, Initialize, &::Zenject::IInitializable::Initialize);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &::System::IDisposable::Dispose);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Tick, &::Zenject::ITickable::Tick);
)
