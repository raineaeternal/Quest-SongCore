#include "SongLoader/SongCoreCustomBeatmapLevelCollection.hpp"

DEFINE_TYPE(SongCore::SongLoader, SongCoreCustomBeatmapLevelCollection);

namespace SongCore::SongLoader {
    void SongCoreCustomBeatmapLevelCollection::ctor() {
        _ctor(ArrayW<GlobalNamespace::CustomPreviewBeatmapLevel*>::Empty());
    }

    SongCoreCustomBeatmapLevelCollection* SongCoreCustomBeatmapLevelCollection::New() {
        return SongCoreCustomBeatmapLevelCollection::New_ctor();
    }
}
