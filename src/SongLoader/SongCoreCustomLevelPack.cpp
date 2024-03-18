#include "SongLoader/CustomLevelPack.hpp"
#include "SongLoader/CustomBeatmapLevelCollection.hpp"

#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include <compare>
#include <string_view>

DEFINE_TYPE(SongCore::SongLoader, CustomLevelPack);

namespace SongCore::SongLoader {
    void CustomLevelPack::ctor(StringW packId, StringW packName, UnityEngine::Sprite* coverImage) {
        _ctor(packId, packName, packName, coverImage, coverImage, CustomBeatmapLevelCollection::New_ctor(), GlobalNamespace::PlayerSensitivityFlag::Unknown);
    }

    CustomLevelPack* CustomLevelPack::New(std::string_view packId, std::string_view packName, UnityEngine::Sprite* coverImage) {
        return CustomLevelPack::New_ctor(packId, packName, coverImage);
    }

    void CustomLevelPack::SortLevels() {
        SortLevels([](auto a, auto b) -> bool { return static_cast<std::u16string_view>(a->songName) < static_cast<std::u16string_view>(b->songName); });
    }

    void CustomLevelPack::SortLevels(WeakSortingFunc sortingFunc) {
        auto levels = ListW<GlobalNamespace::CustomPreviewBeatmapLevel*>(get_beatmapLevelCollection()->beatmapLevels);
        std::stable_sort(levels.begin(), levels.end(), sortingFunc);
    }

    void CustomLevelPack::SetLevels(std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> levels) {
        auto coll = il2cpp_utils::cast<CustomBeatmapLevelCollection>(beatmapLevelCollection);
        auto previewLevels = ListW<GlobalNamespace::CustomPreviewBeatmapLevel*>::New(levels);
        coll->_customPreviewBeatmapLevels = previewLevels->i___System__Collections__Generic__IReadOnlyList_1_T_();
    }
}
