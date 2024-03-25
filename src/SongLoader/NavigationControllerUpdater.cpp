#include "SongLoader/NavigationControllerUpdater.hpp"

#include "GlobalNamespace/LevelCollectionTableView.hpp"
#include "SongLoader/CustomBeatmapLevelsRepository.hpp"
#include "UnityEngine/GameObject.hpp"
#include "logging.hpp"

DEFINE_TYPE(SongCore::SongLoader, NavigationControllerUpdater);

namespace SongCore::SongLoader {
    void NavigationControllerUpdater::ctor(RuntimeSongLoader* runtimeSongLoader, GlobalNamespace::LevelFilteringNavigationController* levelFilteringNavigationController, GlobalNamespace::LevelCollectionViewController* levelCollectionViewController) {
        _runtimeSongLoader = runtimeSongLoader;
        _levelFilteringNavigationController = levelFilteringNavigationController;
        _levelCollectionViewController = levelCollectionViewController;
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

        // get the selected level to keep the selection after the songs update
        auto tableView = _levelCollectionViewController->_levelCollectionTableView;
        auto selectedLevel = tableView->_selectedBeatmapLevel;
        _levelFilteringNavigationController->UpdateCustomSongs();
        if (selectedLevel) { // if no level, don't keep selection
            tableView->SelectLevel(selectedLevel);
        }
    }
}
