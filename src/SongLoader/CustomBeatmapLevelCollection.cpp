#include "SongLoader/CustomBeatmapLevelCollection.hpp"

DEFINE_TYPE(SongCore::SongLoader, CustomBeatmapLevelCollection);

namespace SongCore::SongLoader {
    void CustomBeatmapLevelCollection::ctor() {
        _ctor(ArrayW<GlobalNamespace::CustomPreviewBeatmapLevel*>::Empty());
    }

    CustomBeatmapLevelCollection* CustomBeatmapLevelCollection::New() {
        return CustomBeatmapLevelCollection::New_ctor();
    }
}
