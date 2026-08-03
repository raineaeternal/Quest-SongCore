#pragma once

#include "custom-types/shared/macros.hpp"
#include "System/Object.hpp"
#include "Zenject/IInitializable.hpp"
#include "System/IDisposable.hpp"
#include "Zenject/DiContainer.hpp"
#include "Characteristics.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(SongCore::Overrides, RotationSpawnLinesOverride, System::Object, System::IDisposable*) {
    // container is kept for GameplayCoreSceneSetupData, which is genuinely optional (TryResolve) depending
    // on scene context; SongCore::Characteristics is always available so it's injected directly instead
    DECLARE_CTOR(ctor, Zenject::DiContainer* container, SongCore::Characteristics* characteristics);

    DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &System::IDisposable::Dispose);
};
