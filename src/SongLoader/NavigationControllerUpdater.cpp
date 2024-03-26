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
        INFO("ISMAIN: {}", BSML::MainThreadScheduler::CurrentThreadIsMainThread());
        INFO("_levelFilteringNavigationController: {}", fmt::ptr(_levelFilteringNavigationController));

        auto levelCollectionTableView = _levelCollectionViewController->_levelCollectionTableView;
        auto level = levelCollectionTableView ? levelCollectionTableView->_selectedBeatmapLevel : nullptr;
        _lastSelectedBeatmapLevelId = level ? level->levelID : "";

        auto pack = _levelFilteringNavigationController->selectedBeatmapLevelPack;
        _lastSelectedPackId = pack->packID;

        _lastSelectedCategory = _levelFilteringNavigationController->selectedLevelCategory;

        _levelFilteringNavigationController->_customLevelPacks = nullptr;
        _levelFilteringNavigationController->_annotatedBeatmapLevelCollectionsViewController->ShowLoading();
        _levelCollectionNavigationController->ShowLoading();

        _levelFilteringNavigationController->UpdateCustomSongs();
    }

    void NavigationControllerUpdater::SongsLoaded(std::span<SongLoader::CustomBeatmapLevel* const> levels) {
        INFO("Updating levelFilteringNavigationController");
        // if category doesn't match anymore, return
        if (_levelFilteringNavigationController->selectedLevelCategory != _lastSelectedCategory) {
            INFO("Level category changed, not updating!");
            return;
        }

        BSML::MainThreadScheduler::ScheduleUntil(
            [nav = this->_levelFilteringNavigationController](){
                return nav->_cancellationTokenSource == nullptr;
            },
            [this](){
                INFO("Selecting level & junk: {}", _lastSelectedBeatmapLevelId);
                auto level = _beatmapLevelsModel->GetBeatmapLevel(_lastSelectedBeatmapLevelId);
                auto levelCollectionTableView = _levelCollectionViewController->_levelCollectionTableView;
                if (level && levelCollectionTableView) {
                    levelCollectionTableView->SelectLevel(level);
                }

                auto tableView = levelCollectionTableView ? levelCollectionTableView->_tableView : nullptr;
                auto scrollView = tableView ? tableView->_scrollView : nullptr;
                auto scrollPosition = scrollView ? scrollView->position : 0;

                if (scrollView) scrollView->ScrollTo(scrollPosition, false);
            }
        );

        return;
        // get and set pack info
        auto pack = _levelFilteringNavigationController->selectedBeatmapLevelPack;
        if (pack) {
            // check whether the pack was still in the beatmap levels model
            pack = _beatmapLevelsModel->GetLevelPack(pack->packID);
            if (pack) {
                _levelFilteringNavigationController->_levelPackIdToBeSelectedAfterPresent = pack->packID;
            }
        }

        // get the selected level to keep the selection after the songs update,
        auto levelCollectionTableView = _levelCollectionViewController->_levelCollectionTableView;
        auto level = levelCollectionTableView ? levelCollectionTableView->_selectedBeatmapLevel : nullptr;

        // get and set level info
        if (pack && level) { // check whether it's still in the beatmap levels model
            // check whether the level was still in the beatmap levels model
            _levelCollectionNavigationController->_beatmapLevelToBeSelectedAfterPresent = level = _beatmapLevelsModel->GetBeatmapLevel(level->levelID);
        }



        // thanks metalit for pointing out updatecustomsongs still starts an async thing
        // BSML::MainThreadScheduler::ScheduleUntil(
        //     [nav = this->_levelFilteringNavigationController]() {
        //         return nav->_cancellationTokenSource == nullptr;
        //     },
        //     [levelCollectionTableView, scrollView, pack, scrollPosition, level]() mutable {
        //         if (level && levelCollectionTableView)
        //             levelCollectionTableView->SelectLevel(level);
        //         if (scrollView) scrollView->ScrollTo(scrollPosition, false);
        //     }
        // );
    }
}
