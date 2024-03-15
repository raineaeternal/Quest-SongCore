#pragma once

#include "custom-types/shared/macros.hpp"
#include "CustomJSONData.hpp"
#include "SongCore.hpp"

#include "Zenject/IInitializable.hpp"
#include "System/IDisposable.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"


DECLARE_CLASS_CODEGEN_INTERFACES(SongCore, LevelSelect, System::Object, std::vector<Il2CppClass*>({classof(Zenject::IInitializable*), classof(System::IDisposable*)}),
        DECLARE_CTOR(ctor, GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController);

        DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::StandardLevelDetailViewController*, _levelDetailViewController);

        DECLARE_OVERRIDE_METHOD_MATCH(void, Initialize, &Zenject::IInitializable::Initialize);
        DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &System::IDisposable::Dispose);

        using ChangeDifficultyBeatmapAction = System::Action_2<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::IDifficultyBeatmap*>;
        using ChangeContentAction = System::Action_2<UnityW<GlobalNamespace::StandardLevelDetailViewController>, GlobalNamespace::StandardLevelDetailViewController::ContentType>;
        DECLARE_INSTANCE_FIELD_PRIVATE(ChangeDifficultyBeatmapAction*, _changeDifficultyBeatmapAction);
        DECLARE_INSTANCE_FIELD_PRIVATE(ChangeContentAction*, _changeContentAction);

    public:
        using LevelWasSelectedEventArgs = SongCore::API::LevelSelect::LevelWasSelectedEventArgs;

        UnorderedEventCallback<LevelWasSelectedEventArgs const&> LevelWasSelected;

        GlobalNamespace::IBeatmapLevelPack* GetSelectedLevelPack();
        __declspec(property(get=GetSelectedLevelPack)) GlobalNamespace::IBeatmapLevelPack* SelectedLevelPack;

        GlobalNamespace::IPreviewBeatmapLevel* GetSelectedPreviewBeatmapLevel();
        __declspec(property(get=GetSelectedPreviewBeatmapLevel)) GlobalNamespace::IPreviewBeatmapLevel* SelectedPreviewBeatmapLevel;

        GlobalNamespace::IBeatmapLevel* GetSelectedBeatmapLevel();
        __declspec(property(get=GetSelectedBeatmapLevel)) GlobalNamespace::IBeatmapLevel* SelectedBeatmapLevel;

        GlobalNamespace::IDifficultyBeatmapSet* GetSelectedDifficultyBeatmapSet();
        __declspec(property(get=GetSelectedDifficultyBeatmapSet)) GlobalNamespace::IDifficultyBeatmapSet* SelectedDifficultyBeatmapSet;

        GlobalNamespace::IDifficultyBeatmap* GetSelectedDifficultyBeatmap();
        __declspec(property(get=GetSelectedDifficultyBeatmap)) GlobalNamespace::IDifficultyBeatmap* SelectedDifficultyBeatmap;

    private:
        void LevelDetailContentChanged(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, GlobalNamespace::StandardLevelDetailViewController::ContentType contentType);
        void BeatmapLevelSelected(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, GlobalNamespace::IDifficultyBeatmap* selectedBeatmap);

        void HandleCustomLevelWasSelected(LevelWasSelectedEventArgs& eventArgs);

        /// @brief invoker method for the events
        void InvokeLevelWasSelected(LevelWasSelectedEventArgs const& eventArgs);
)
