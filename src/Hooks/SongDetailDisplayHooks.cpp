#include "hooking.hpp"
#include "logging.hpp"

#include "GlobalNamespace/LevelListTableCell.hpp"
#include "SongLoader/CustomBeatmapLevel.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Transform.hpp"
#include "TMPro/TMP_Text.hpp"

MAKE_AUTO_HOOK_MATCH(
    LevelListTableCell_SetDataFromLevelAsync,
    &GlobalNamespace::LevelListTableCell::SetDataFromLevelAsync,
    void,
    GlobalNamespace::LevelListTableCell* self,
    GlobalNamespace::BeatmapLevel* level,
    bool isFavorite,
    bool isPromoted,
    bool isUpdated
) {
    LevelListTableCell_SetDataFromLevelAsync(self, level, isFavorite, isPromoted, isUpdated);
    if (!level) return;

    self->_songBpmText->text = std::to_string((int)level->beatsPerMinute);
    auto customLevel = il2cpp_utils::try_cast<SongCore::SongLoader::CustomBeatmapLevel>(level);
    if (customLevel.has_value()) {
        self->_songAuthorText->richText = true;
        if (!System::String::IsNullOrWhiteSpace(level->allMappers[0])) {
            auto songAuthorName = level->songAuthorName;
            auto levelAuthorName = level->allMappers[0];

            auto color = "ff69b4";

            self->_songAuthorText->text = fmt::format("<size=80%><noparse>{}</noparse></size> <size=90%>[<color=#{}><noparse>{}</noparse></color>]</size>", songAuthorName, color, levelAuthorName);
        }
    }

    if (isFavorite) {
        static auto favoriteSize = UnityEngine::Vector3(1.4,1.4,1.4);

        UnityEngine::Color color = {1, 1, 1, 1};
        if (customLevel.has_value()) color = {1, 0.4, 0.7, 0.7};
        self->_favoritesBadgeImage->color = color;
        self->_favoritesBadgeImage->transform->localScale = favoriteSize;
    }
}
