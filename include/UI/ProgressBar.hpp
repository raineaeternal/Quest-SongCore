#pragma once

#include "SongLoader/SongLoader.hpp"
#include "custom-types/shared/macros.hpp"
#include "lapiz/shared/macros.hpp"
#include "bsml/shared/BSML.hpp"
#include "UnityEngine/MonoBehaviour.hpp"
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

#include "SongLoader/SongLoader.hpp"

DECLARE_CLASS_CODEGEN(SongCore::UI, ProgressBar, UnityEngine::MonoBehaviour,
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

    std::string HeaderText = "Loading songs...";
    std::string PluginText = "SongCore Loader";

    float headerTextSize = 15.0f;
    float pluginTextSize = 9.0f;


    DECLARE_PRIVATE_METHOD(void, Awake);
    DECLARE_PRIVATE_METHOD(void, Update);
)
