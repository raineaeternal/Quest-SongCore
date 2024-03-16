#include "hooking.hpp"
#include "logging.hpp"

#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionCell.hpp"
#include "SongLoader/SongCoreCustomLevelPack.hpp"

// sets the download icon visibility to false if this is a custom pack
MAKE_AUTO_HOOK_MATCH(AnnotatedBeatmapLevelCollectionCell_RefreshAvailabilityAsync, &GlobalNamespace::AnnotatedBeatmapLevelCollectionCell::RefreshAvailabilityAsync, void, GlobalNamespace::AnnotatedBeatmapLevelCollectionCell* self, ::GlobalNamespace::IAdditionalContentModel* contentModel) {
    AnnotatedBeatmapLevelCollectionCell_RefreshAvailabilityAsync(self, contentModel);
    if (il2cpp_utils::try_cast<SongCore::SongLoader::SongCoreCustomLevelPack>(self->_annotatedBeatmapLevelCollection).has_value()) {
        self->SetDownloadIconVisible(false);
    }
}
