#include "SongLoader/NavigationControllerUpdater.hpp"

#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "SongLoader/CustomBeatmapLevelsRepository.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "UnityEngine/GameObject.hpp"
#include "HMUI/TableView.hpp"
#include "HMUI/ScrollView.hpp"
#include "logging.hpp"

DEFINE_TYPE(SongCore::SongLoader, NavigationControllerUpdater);

namespace SongCore::SongLoader {
    void NavigationControllerUpdater::ctor(GlobalNamespace::BeatmapLevelsModel* beatmapLevelsModel, RuntimeSongLoader* runtimeSongLoader, GlobalNamespace::LevelFilteringNavigationController* levelFilteringNavigationController, GlobalNamespace::LevelCollectionNavigationController* levelCollectionNavigationController, GlobalNamespace::LevelCollectionViewController* levelCollectionViewController) {
        _runtimeSongLoader = runtimeSongLoader;
        _levelFilteringNavigationController = levelFilteringNavigationController;
        _levelCollectionNavigationController = levelCollectionNavigationController;
        _levelCollectionViewController = levelCollectionViewController;
        _beatmapLevelsModel = beatmapLevelsModel;
    }

    void NavigationControllerUpdater::Initialize() {
        _runtimeSongLoader->CustomLevelPacksRefreshed += {&NavigationControllerUpdater::CustomLevelPacksRefreshed, this};
        if (_runtimeSongLoader->AreSongsLoaded) CustomLevelPacksRefreshed(_runtimeSongLoader->CustomBeatmapLevelsRepository);
    }

    void NavigationControllerUpdater::Dispose() {
        _runtimeSongLoader->CustomLevelPacksRefreshed -= {&NavigationControllerUpdater::CustomLevelPacksRefreshed, this};
    }

    void NavigationControllerUpdater::CustomLevelPacksRefreshed(CustomBeatmapLevelsRepository* collection) {
        INFO("Updating levelFilteringNavigationController");
        // if this is null, the controller wasn't setup yet, just make sure the packs are setup
        if (!_levelFilteringNavigationController->_allBeatmapLevelPacks) _levelFilteringNavigationController->SetupBeatmapLevelPacks();

        GlobalNamespace::BeatmapLevelPack* packToSelect = nullptr;

        // get and set pack info
        auto pack = _levelFilteringNavigationController->selectedBeatmapLevelPack;
        if (pack) {
            // check whether the pack was still in the beatmap levels model
            pack = _beatmapLevelsModel->GetLevelPack(pack->packID);
            if (packToSelect) {
                _levelFilteringNavigationController->_levelPackIdToBeSelectedAfterPresent = pack->packID;
            }
        }

        // get the selected level to keep the selection after the songs update,
        auto levelCollectionTableView = _levelCollectionViewController->_levelCollectionTableView;
        auto selectedLevel = levelCollectionTableView ? levelCollectionTableView->_selectedBeatmapLevel : nullptr;

        // get and set level info
        if (packToSelect && selectedLevel) { // check whether it's still in the beatmap levels model
            // check whether the level was still in the beatmap levels model
            _levelCollectionNavigationController->_beatmapLevelToBeSelectedAfterPresent = _beatmapLevelsModel->GetBeatmapLevel(selectedLevel->levelID);
        }

        auto tableView = levelCollectionTableView ? levelCollectionTableView->_tableView : nullptr;
        auto scrollView = tableView ? tableView->_scrollView : nullptr;
        auto pos = scrollView ? scrollView->position : 0;

        _levelFilteringNavigationController->UpdateCustomSongs();

        if (scrollView) scrollView->ScrollTo(pos, false);
    }
}
