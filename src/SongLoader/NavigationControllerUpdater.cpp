#include "SongLoader/NavigationControllerUpdater.hpp"
#include "logging.hpp"

DEFINE_TYPE(SongCore::SongLoader, NavigationControllerUpdater);

namespace SongCore::SongLoader {
    void NavigationControllerUpdater::ctor(RuntimeSongLoader* runtimeSongLoader, GlobalNamespace::LevelFilteringNavigationController* levelFilteringNavigationController) {
        _runtimeSongLoader = runtimeSongLoader;
        _levelFilteringNavigationController = levelFilteringNavigationController;
    }

    void NavigationControllerUpdater::Initialize() {
        _runtimeSongLoader->CustomLevelPacksRefreshed += {&NavigationControllerUpdater::CustomLevelPacksRefreshed, this};
        if (_runtimeSongLoader->AreSongsLoaded) CustomLevelPacksRefreshed(_runtimeSongLoader->CustomBeatmapLevelPackCollection);
    }

    void NavigationControllerUpdater::Dispose() {
        _runtimeSongLoader->CustomLevelPacksRefreshed -= {&NavigationControllerUpdater::CustomLevelPacksRefreshed, this};
    }

    void NavigationControllerUpdater::CustomLevelPacksRefreshed(CustomBeatmapLevelPackCollection* collection) {
        INFO("Updating levelFilteringNavigationController");
        _levelFilteringNavigationController->UpdateCustomSongs();
    }
}
