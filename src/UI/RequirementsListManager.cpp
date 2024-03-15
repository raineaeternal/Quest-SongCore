#include "UI/RequirementsListManager.hpp"

#include "CustomJSONData.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/EnvironmentInfoSO.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/RectTransform.hpp"

#include "logging.hpp"
#include "config.hpp"
#include "assets.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/Helpers/delegates.hpp"

DEFINE_TYPE(SongCore::UI, RequirementsListManager);

static inline UnityEngine::Vector3 operator*(UnityEngine::Vector3 vec, float v) {
    return {
        vec.x * v,
        vec.y * v,
        vec.z * v
    };
}

namespace SongCore::UI {
    void RequirementsListManager::ctor(GlobalNamespace::StandardLevelDetailViewController* levelDetailViewController, ColorsOptions* colorsOptions, Capabilities* capabilities, LevelSelect* levelSelect, IconCache* iconCache) {
        _levelDetailViewController = levelDetailViewController;
        _colorsOptions = colorsOptions;
        _capabilities = capabilities;
        _levelSelect = levelSelect;
        _iconCache = iconCache;

        _requirementsCells = ListW<BSML::CustomCellInfo*>::New();
        _unusedRequirementCells = ListW<BSML::CustomCellInfo*>::New();
        _showColorsOnModalHideAction = BSML::MakeSystemAction<>(std::bind(&RequirementsListManager::ShowColorsOptions, this));
    }

    void RequirementsListManager::Initialize() {
        _levelSelect->LevelWasSelected += {&RequirementsListManager::LevelWasSelected, this};

        auto levelDetailView = _levelDetailViewController->_standardLevelDetailView;
        BSML::parse_and_construct("<bg><action-button id='_requirementButton' text='?' anchor-pos-x='31' anchor-pos-y='0' pref-width='12' pref-height='9' on-click='ShowRequirements'/></bg>", levelDetailView->transform, this);
        _requirementButton->transform->parent->localScale *= 0.7f;
        _colorsOptions->onModalHide = std::bind(&RequirementsListManager::ShowRequirements, this);

        levelDetailView->_favoriteToggle->transform.cast<UnityEngine::RectTransform>()->anchoredPosition = {3, -2};
    }

    void RequirementsListManager::Dispose() {
        _levelSelect->LevelWasSelected -= {&RequirementsListManager::LevelWasSelected, this};
    }

    void RequirementsListManager::UpdateRequirementButton() {
        _requirementButton->gameObject->SetActive(!_requirementsCells.empty());
    }

    BSML::CustomCellInfo* RequirementsListManager::GetCellInfo() {
        if (_unusedRequirementCells.empty()) return BSML::CustomCellInfo::New_ctor();

        auto lastIdx = _unusedRequirementCells.size() - 1;
        auto last = _unusedRequirementCells[lastIdx];
        _unusedRequirementCells->Remove(last);
        return last;
    }

    void RequirementsListManager::ClearRequirementCells() {
        for (auto cellInfo : _requirementsCells) _unusedRequirementCells->Add(cellInfo);
        _requirementsCells->Clear();
    }

    void RequirementsListManager::UpdateRequirementCells(LevelSelect::LevelWasSelectedEventArgs const& eventArgs) {
        DEBUG("Updating requirement cells");
        ClearRequirementCells();

        if (!eventArgs.isCustom) {
            DEBUG("Last selected level was not custom! returning...");
            return;
        }

        auto& levelDetails = eventArgs.customLevelDetails->levelDetails;
        auto& characteristicDetails = eventArgs.customLevelDetails->characteristicDetails;
        auto& diffDetails = eventArgs.customLevelDetails->difficultyDetails;

        for (auto& requirement : diffDetails.requirements) {
            static ConstString RequirementFound("Requirement Found");
            static ConstString RequirementMissing("Requirement Missing");

            bool installed = _capabilities->IsCapabilityRegistered(requirement);
            auto cell = GetCellInfo();
            cell->text = fmt::format("<size=75%>{}", requirement);
            cell->subText = installed ? RequirementFound : RequirementMissing;
            cell->icon = installed ? _iconCache->HaveReqIcon : _iconCache->MissingReqIcon;
            _requirementsCells.push_back(cell);
        }

        for (auto& contributor : levelDetails.contributors) {
            auto cell = GetCellInfo();
            cell->text = fmt::format("<size=75%>{}", contributor.name);
            cell->subText = contributor.role;
            UnityEngine::Sprite* icon = nullptr;
            if (!contributor.iconPath.empty()) {
                std::filesystem::path levelPath(eventArgs.customPreviewBeatmapLevel->customLevelPath);
                icon = _iconCache->GetIconForPath(levelPath / contributor.iconPath);
            }
            cell->icon = icon ? icon : _iconCache->InfoIcon;
            _requirementsCells.push_back(cell);
        }

        if (eventArgs.isWIP) {
            static ConstString WipText("<size=75%>WIP Level. Please Play in Practice Mode");
            static ConstString WipSubText("Warning");

            auto cell = GetCellInfo();
            cell->text = WipText;
            cell->subText = WipSubText;
            cell->icon = _iconCache->WarningIcon;
            _requirementsCells.push_back(cell);
        }

        if (diffDetails.customColors.has_value()) {
            static ConstString CustomColorsText("<size=75%>Custom Colors Available");
            static ConstString CustomColorsSubText("Click here to preview & enable or disable it.");

            auto cell = GetCellInfo();
            cell->text = CustomColorsText;
            cell->subText = CustomColorsSubText;
            cell->icon = _iconCache->ColorsIcon;
            _requirementsCells.push_back(cell);

            _colorsOptions->Parse();
            _colorsOptions->SetColors(diffDetails.customColors.value());
        }

        for (auto& warning : diffDetails.warnings) {
            static ConstString Warning("Warning");

            auto cell = GetCellInfo();
            cell->text = fmt::format("<size=75%>{}", warning);
            cell->subText = Warning;
            cell->icon = _iconCache->WarningIcon;
            _requirementsCells.push_back(cell);
        }

        for (auto& information : diffDetails.information) {
            static ConstString Info("Info");

            auto cell = GetCellInfo();
            cell->text = fmt::format("<size=75%>{}", information);
            cell->subText = Info;
            cell->icon = _iconCache->InfoIcon;
            _requirementsCells.push_back(cell);
        }

        for (auto& suggestion : diffDetails.suggestions) {
            static ConstString SuggestionFound("Suggestion Found");
            static ConstString SuggestionMissing("Suggestion Missing");

            bool installed = _capabilities->IsCapabilityRegistered(suggestion);
            auto cell = GetCellInfo();
            cell->text = suggestion;
            cell->subText = installed ? SuggestionFound : SuggestionMissing;
            cell->icon = installed ? _iconCache->HaveSuggestionIcon : _iconCache->MissingSuggestionIcon;
            _requirementsCells.push_back(cell);
        }

        if (diffDetails.oneSaber.has_value()) {
            auto oneSaber = diffDetails.oneSaber.value();
            static const char* FANCY_ENABLED("[<color=#89ff89>Enabled</color]");
            static const char* FANCY_DISABLED("[<color=#ff5072>Disabled</color]");
            static const char* ENABLE("enable");
            static const char* DISABLE("disable");
            static const char* FORCED_ONE("Forced One Saber");
            static const char* FORCED_TWO("Forced Standard");

            auto enabledText = config.disableOneSaberOverride ? FANCY_DISABLED : FANCY_ENABLED;
            auto enableSubText = config.disableOneSaberOverride ? ENABLE : DISABLE;
            auto saberCountText = oneSaber ? FORCED_ONE : FORCED_TWO;

            auto cell = GetCellInfo();
            cell->text = fmt::format("<size=75%>{} {}", saberCountText, enabledText);
            cell->subText = fmt::format("Map changes saber count, click here to {}", enableSubText);
            cell->icon = oneSaber ? _iconCache->OneSaberIcon : _iconCache->StandardIcon;
            _requirementsCells.push_back(cell);
        }

        // add environment info here if list isn't empty
        if (!_requirementsCells.empty()) {
            static ConstString EnvironmentInfo("<size=75%>Environment");

            auto cell = GetCellInfo();
            cell->text = EnvironmentInfo;
            cell->subText = eventArgs.previewBeatmapLevel->environmentInfo->environmentName;
            cell->icon = _iconCache->EnvironmentIcon;
            _requirementsCells.push_back(cell);
        }
    }

    void RequirementsListManager::ShowRequirements() {
        if (!_requirementModal) {
            BSML::parse_and_construct(Assets::requirements_bsml, _requirementButton->transform, this);
        }

        _listTableData->tableView->ReloadData();
        _listTableData->tableView->ScrollToCellWithIdx(0, ::HMUI::TableView::ScrollPositionType::Beginning, true);

        _requirementModal->onHide = nullptr;
        _requirementModal->Show();
    }

    void RequirementsListManager::ShowColorsOptions() {
        _colorsOptions->Show();
    }

    void RequirementsListManager::RequirementCellSelected(HMUI::TableView* tableView, int cellIndex) {
        auto cell = _requirementsCells[cellIndex];
        auto selectedIcon = cell->icon;

        if (selectedIcon == _iconCache->ColorsIcon) {
            _requirementModal->HMUI::ModalView::Hide(false, _showColorsOnModalHideAction);
        } else if (selectedIcon == _iconCache->StandardIcon || selectedIcon == _iconCache->OneSaberIcon) {
            config.disableOneSaberOverride = !config.disableOneSaberOverride;
            SaveConfig();
            _requirementModal->Hide();
        }
        _listTableData->tableView->ClearSelection();
    }

    void RequirementsListManager::LevelWasSelected(LevelSelect::LevelWasSelectedEventArgs const& eventArgs) {
        UpdateRequirementCells(eventArgs);
        UpdateRequirementButton();
    }

    ListW<BSML::CustomCellInfo*> RequirementsListManager::get_requirementsCells() {
        return _requirementsCells;
    }
}
