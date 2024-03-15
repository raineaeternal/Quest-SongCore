#pragma once

#include "CustomJSONData.hpp"
#include "custom-types/shared/macros.hpp"
#include "System/Object.hpp"
#include "Zenject/IInitializable.hpp"
#include "System/IDisposable.hpp"
#include "UnityEngine/UI/Button.hpp"

#include "SongLoader/RuntimeSongLoader.hpp"
#include "PlayButtonInteractable.hpp"
#include "Capabilities.hpp"
#include "LevelSelect.hpp"
#include "CustomJSONData.hpp"

#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(SongCore::UI, PlayButtonsUpdater, System::Object, std::vector<Il2CppClass*>({classof(Zenject::IInitializable*), classof(System::IDisposable*)}),
    DECLARE_CTOR(ctor, GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, PlayButtonInteractable* playButtonInteractable, Capabilities* capabilities, LevelSelect* levelSelect);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::StandardLevelDetailViewController*, _levelDetailViewController);
    DECLARE_INSTANCE_FIELD_PRIVATE(PlayButtonInteractable*, _playButtonInteractable);
    DECLARE_INSTANCE_FIELD_PRIVATE(Capabilities*, _capabilities);
    DECLARE_INSTANCE_FIELD_PRIVATE(LevelSelect*, _levelSelect);

    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::CustomBeatmapLevel*, _lastSelectedCustomLevel);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::IDifficultyBeatmap*, _lastSelectedDifficultyBeatmap);

    DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::UI::Button*, _playButton);
    DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::UI::Button*, _practiceButton);

    DECLARE_OVERRIDE_METHOD_MATCH(void, Initialize, &Zenject::IInitializable::Initialize);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &System::IDisposable::Dispose);

    private:
        void LevelWasSelected(LevelSelect::LevelWasSelectedEventArgs const& eventArgs);

        bool IsPlayerAllowedToStart();
        void UpdatePlayButtonsState();

        /// @brief whether there are any disabling mod infos
        bool _anyDisablingModInfos;
        bool _levelIsCustom;
        bool _levelIsWIP;
        bool _missingRequirements;
        void HandleDisablingModInfosChanged(std::span<PlayButtonInteractable::PlayButtonDisablingModInfo const> disablingModInfos);
)
