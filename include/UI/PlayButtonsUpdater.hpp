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
#include "CustomJSONData.hpp"

#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(SongCore::UI, PlayButtonsUpdater, System::Object, std::vector<Il2CppClass*>({classof(Zenject::IInitializable*), classof(System::IDisposable*)}),
    DECLARE_CTOR(ctor, GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, PlayButtonInteractable* playButtonInteractable, Capabilities* capabilities);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::StandardLevelDetailViewController*, _levelDetailViewController);
    DECLARE_INSTANCE_FIELD_PRIVATE(PlayButtonInteractable*, _playButtonInteractable);
    DECLARE_INSTANCE_FIELD_PRIVATE(Capabilities*, _capabilities);

    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::CustomBeatmapLevel*, _lastSelectedCustomLevel);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::IDifficultyBeatmap*, _lastSelectedDifficultyBeatmap);

    DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::UI::Button*, _playButton);
    DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::UI::Button*, _practiceButton);

    using ChangeDifficultyBeatmapAction = System::Action_2<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::IDifficultyBeatmap*>;
    using ChangeContentAction = System::Action_2<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::StandardLevelDetailViewController::ContentType>;
    DECLARE_INSTANCE_FIELD_PRIVATE(ChangeDifficultyBeatmapAction*, _changeDifficultyBeatmapAction);
    DECLARE_INSTANCE_FIELD_PRIVATE(ChangeContentAction*, _changeContentAction);

    DECLARE_OVERRIDE_METHOD_MATCH(void, Initialize, &Zenject::IInitializable::Initialize);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &System::IDisposable::Dispose);

    private:
        void LevelDetailContentChanged(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, GlobalNamespace::StandardLevelDetailViewController::ContentType contentType);
        void BeatmapLevelSelected(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, GlobalNamespace::IDifficultyBeatmap* selectedBeatmap);

        void HandleVanillaLevelWasSelected(GlobalNamespace::IBeatmapLevel* level, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap);
        void HandleCustomLevelWasSelected(GlobalNamespace::CustomBeatmapLevel* level, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap);

        bool IsPlayerAllowedToStart();
        void UpdatePlayButtonsState();

        /// @brief whether there are any disabling mod infos
        bool _anyDisablingModInfos;
        void HandleDisablingModInfosChanged(std::span<PlayButtonInteractable::PlayButtonDisablingModInfo const> disablingModInfos);
)
