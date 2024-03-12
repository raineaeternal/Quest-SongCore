#include "SongLoader/SongCoreCustomLevelPack.hpp"
#include "SongLoader/SongCoreCustomBeatmapLevelCollection.hpp"

#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"
#include <compare>
#include <string_view>

DEFINE_TYPE(SongCore::SongLoader, SongCoreCustomLevelPack);

namespace SongCore::SongLoader {
    void SongCoreCustomLevelPack::ctor(StringW packId, StringW packName, UnityEngine::Sprite* coverImage) {
        _ctor(packId, packName, packName, coverImage, coverImage, SongCoreCustomBeatmapLevelCollection::New_ctor(), GlobalNamespace::PlayerSensitivityFlag::Unknown);
    }

    SongCoreCustomLevelPack* SongCoreCustomLevelPack::New(std::string_view packId, std::string_view packName, UnityEngine::Sprite* coverImage) {
        return SongCoreCustomLevelPack::New_ctor(packId, packName, coverImage);
    }

    void SongCoreCustomLevelPack::SortLevels() {
        SortLevels([](auto a, auto b) -> bool { return static_cast<std::u16string_view>(a->songName) < static_cast<std::u16string_view>(b->songName); });
    }

    void SongCoreCustomLevelPack::SortLevels(WeakSortingFunc sortingFunc) {
        auto levels = ListW<GlobalNamespace::CustomPreviewBeatmapLevel*>(get_beatmapLevelCollection()->beatmapLevels);
        std::stable_sort(levels.begin(), levels.end(), sortingFunc);
    }

    void SongCoreCustomLevelPack::SetLevels(std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> levels) {
        auto coll = il2cpp_utils::cast<SongCoreCustomBeatmapLevelCollection>(beatmapLevelCollection);
        auto previewLevels = ListW<GlobalNamespace::CustomPreviewBeatmapLevel*>::New(levels);
        coll->_customPreviewBeatmapLevels = previewLevels->i___System__Collections__Generic__IReadOnlyList_1_T_();
    }
}
