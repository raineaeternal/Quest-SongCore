#pragma once

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "Zenject/IInitializable.hpp"
#include "Zenject/ITickable.hpp"
#include "System/IDisposable.hpp"

#include "SongLoader/RuntimeSongLoader.hpp"
#include "LevelSelect.hpp"
#include "IconCache.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(SongCore::UI, DeleteLevelButton, System::Object, std::vector<Il2CppClass*>({classof(Zenject::IInitializable*), classof(Zenject::ITickable*), classof(System::IDisposable*)}),
        DECLARE_CTOR(ctor, SongLoader::RuntimeSongLoader* runtimeSongLoader, GlobalNamespace::StandardLevelDetailViewController* standardLevelDetailViewController, LevelSelect* levelSelect, IconCache* iconCache);
        DECLARE_INSTANCE_FIELD_PRIVATE(SongLoader::RuntimeSongLoader*, _runtimeSongLoader);
        DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::StandardLevelDetailViewController*, _levelDetailViewController);
        DECLARE_INSTANCE_FIELD_PRIVATE(LevelSelect*, _levelSelect);
        DECLARE_INSTANCE_FIELD_PRIVATE(IconCache*, _iconCache);

        DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::GameObject*, _deleteButtonRoot);
        DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::UI::Button*, _deleteButton);

        DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::CustomBeatmapLevel*, _lastSelectedCustomLevel);
        DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::IDifficultyBeatmap*, _lastSelectedDifficultyBeatmap);

        DECLARE_OVERRIDE_METHOD_MATCH(void, Initialize, &Zenject::IInitializable::Initialize);
        DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &System::IDisposable::Dispose);
        DECLARE_OVERRIDE_METHOD_MATCH(void, Tick, &Zenject::ITickable::Tick);
        DECLARE_INSTANCE_METHOD(void, AttemptDeleteCurrentlySelectedLevel);

    private:
        std::future<void> _songDeleteFuture;
        void UpdateButtonState();

        void LevelWasSelected(LevelSelect::LevelWasSelectedEventArgs const& eventArgs);
)
