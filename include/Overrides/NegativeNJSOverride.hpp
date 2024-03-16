#pragma once

#include "custom-types/shared/macros.hpp"
#include "System/Object.hpp"
#include "Zenject/IInitializable.hpp"
#include "System/IDisposable.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(SongCore::Overrides, NegativeNJSOverride, System::Object, classof(System::IDisposable*),
    DECLARE_CTOR(ctor, GlobalNamespace::GameplayCoreSceneSetupData* sceneSetupData);

    DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &System::IDisposable::Dispose);
)
