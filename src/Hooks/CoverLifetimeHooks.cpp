#include "hooking.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/IPreviewMediaData.hpp"
#include "GlobalNamespace/LevelBar.hpp"

// SetupData releases earlier covers; OnDestroy must release the final request.
MAKE_AUTO_HOOK_MATCH(LevelBar_ReleaseCoverOnDestroy, &GlobalNamespace::LevelBar::OnDestroy, void, GlobalNamespace::LevelBar* self) {
    LevelBar_ReleaseCoverOnDestroy(self);
    auto level = self->_beatmapLevel;
    self->_beatmapLevel = nullptr;
    if (level && level->previewMediaData)
        level->previewMediaData->UnloadCoverSprite();
}
