#include "SongLoader/NavigationControllerUpdater.hpp"

#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "SongLoader/CustomBeatmapLevelsRepository.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Action_1.hpp"
#include "UnityEngine/GameObject.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/ScrollView.hpp"
#include "logging.hpp"

#include "bsml/shared/BSML/MainThreadScheduler.hpp"

DEFINE_TYPE(SongCore::SongLoader, NavigationControllerUpdater);

namespace SongCore::SongLoader {
    void NavigationControllerUpdater::ctor(GlobalNamespace::BeatmapLevelsModel* beatmapLevelsModel, RuntimeSongLoader* runtimeSongLoader, GlobalNamespace::LevelFilteringNavigationController* levelFilteringNavigationController, GlobalNamespace::LevelCollectionNavigationController* levelCollectionNavigationController, GlobalNamespace::LevelCollectionViewController* levelCollectionViewController) {
        _runtimeSongLoader = runtimeSongLoader;
        _levelFilteringNavigationController = levelFilteringNavigationController;
        _levelCollectionNavigationController = levelCollectionNavigationController;
        _levelCollectionViewController = levelCollectionViewController;
        _beatmapLevelsModel = beatmapLevelsModel;

        _lastSelectedBeatmapLevelId = "";
        _lastSelectedPackId = "";
    }

    void NavigationControllerUpdater::Initialize() {
        _runtimeSongLoader->SongsWillRefresh += {&NavigationControllerUpdater::SongsWillRefresh, this};
        _runtimeSongLoader->SongsLoaded += {&NavigationControllerUpdater::SongsLoaded, this};
        if (_runtimeSongLoader->AreSongsLoaded) {
            SongsLoaded(_runtimeSongLoader->AllLevels);
        }
    }

    void NavigationControllerUpdater::Dispose() {
        _runtimeSongLoader->SongsWillRefresh -= {&NavigationControllerUpdater::SongsWillRefresh, this};
        _runtimeSongLoader->SongsLoaded -= {&NavigationControllerUpdater::SongsLoaded, this};
    }

    void NavigationControllerUpdater::SongsWillRefresh() {
        auto levelCollectionTableView = _levelCollectionViewController->_levelCollectionTableView;
        auto level = levelCollectionTableView ? levelCollectionTableView->_selectedBeatmapLevel : nullptr;
        _lastSelectedBeatmapLevelId = level ? level->levelID : "";

        auto pack = _levelFilteringNavigationController->selectedBeatmapLevelPack;
        _lastSelectedPackId = pack ? pack->packID : "";

        _lastSelectedCategory = _levelFilteringNavigationController->selectedLevelCategory;

        auto tableView = levelCollectionTableView ? levelCollectionTableView->_tableView : nullptr;
        auto scrollView = tableView ? tableView->_scrollView : nullptr;
        _lastScrollPosition = scrollView ? scrollView->position : 0;

        _levelFilteringNavigationController->_customLevelPacks = nullptr;
        _levelFilteringNavigationController->_annotatedBeatmapLevelCollectionsViewController->ShowLoading();
        _levelCollectionNavigationController->ShowLoading();

        _levelFilteringNavigationController->UpdateCustomSongs();
    }

    void NavigationControllerUpdater::SongsLoaded(std::span<SongLoader::CustomBeatmapLevel* const> levels) {
        // if category doesn't match anymore, return
        if (_levelFilteringNavigationController->selectedLevelCategory != _lastSelectedCategory || _lastSelectedCategory == GlobalNamespace::SelectLevelCategoryViewController::LevelCategory::None) {
            INFO("Level category changed or was none, not updating!");
            return;
        }

        auto pack = _beatmapLevelsModel->GetLevelPack(_lastSelectedPackId);
        if (pack) {
            INFO("Setting pack '{}' to be selected after present", _lastSelectedPackId);
            _levelFilteringNavigationController->_levelPackIdToBeSelectedAfterPresent = _lastSelectedPackId;
        }

        // thanks metalit for pointing out updatecustomsongs still starts an async thing, doing things this way lets us await the reload to be complete
        BSML::MainThreadScheduler::ScheduleUntil(
            [nav = this->_levelFilteringNavigationController](){
                return nav->_cancellationTokenSource == nullptr;
            },
            [this](){
                INFO("Selecting level '{}'", _lastSelectedBeatmapLevelId);

                auto level = _beatmapLevelsModel->GetBeatmapLevel(_lastSelectedBeatmapLevelId);
                auto levelCollectionTableView = _levelCollectionViewController->_levelCollectionTableView;
                if (level && levelCollectionTableView) {
                    levelCollectionTableView->SelectLevel(level);
                }

                auto tableView = levelCollectionTableView ? levelCollectionTableView->_tableView : nullptr;
                auto scrollView = tableView ? tableView->_scrollView : nullptr;

                if (scrollView) scrollView->ScrollTo(_lastScrollPosition, false);
            }
        );
    }
}
