#pragma once

#include "custom-types/shared/macros.hpp"
#include "Zenject/IInitializable.hpp"
#include "System/IDisposable.hpp"

#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/GameObject.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/CustomBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"

#include "LevelSelect.hpp"
#include "Capabilities.hpp"
#include "IconCache.hpp"
#include "ColorsOptions.hpp"

#include "bsml/shared/BSML/Components/CustomListTableData.hpp"
#include "bsml/shared/BSML/Components/ModalView.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(SongCore::UI, RequirementsListManager, System::Object, std::vector<Il2CppClass*>({classof(Zenject::IInitializable*), classof(System::IDisposable*)}),
    DECLARE_CTOR(ctor, GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, ColorsOptions* colorsOptions, Capabilities* capabilities, LevelSelect* levelSelect, IconCache* _iconCache);

    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::StandardLevelDetailViewController*, _levelDetailViewController);
    DECLARE_INSTANCE_FIELD_PRIVATE(ColorsOptions*, _colorsOptions);
    DECLARE_INSTANCE_FIELD_PRIVATE(Capabilities*, _capabilities);
    DECLARE_INSTANCE_FIELD_PRIVATE(LevelSelect*, _levelSelect);
    DECLARE_INSTANCE_FIELD_PRIVATE(IconCache*, _iconCache);

    DECLARE_INSTANCE_FIELD_PRIVATE(UnityEngine::UI::Button*, _requirementButton);
    DECLARE_INSTANCE_FIELD_PRIVATE(BSML::CustomListTableData*, _listTableData);
    DECLARE_INSTANCE_FIELD_PRIVATE(BSML::ModalView*, _requirementModal);
    DECLARE_INSTANCE_FIELD_PRIVATE(System::Action*, _showColorsOnModalHideAction);

    DECLARE_INSTANCE_FIELD_PRIVATE(ListW<BSML::CustomCellInfo*>, _requirementsCells);
    DECLARE_INSTANCE_FIELD_PRIVATE(ListW<BSML::CustomCellInfo*>, _unusedRequirementCells);

    DECLARE_OVERRIDE_METHOD_MATCH(void, Initialize, &Zenject::IInitializable::Initialize);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &System::IDisposable::Dispose);

    DECLARE_INSTANCE_METHOD(void, ShowRequirements);

    DECLARE_INSTANCE_METHOD(void, RequirementCellSelected, HMUI::TableView* tableView, int cellIndex);
    DECLARE_INSTANCE_METHOD(ListW<BSML::CustomCellInfo*>, get_requirementsCells);
    private:
        BSML::CustomCellInfo* GetCellInfo();
        void ClearRequirementCells();
        void UpdateRequirementCells(LevelSelect::LevelWasSelectedEventArgs const& eventArgs);

        void LevelWasSelected(LevelSelect::LevelWasSelectedEventArgs const& eventArgs);

        void ShowColorsOptions();
        void UpdateRequirementButton();
)
