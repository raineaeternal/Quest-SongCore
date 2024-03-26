#include "SongLoader/CustomLevelPack.hpp"

#include <compare>
#include <string_view>

DEFINE_TYPE(SongCore::SongLoader, CustomLevelPack);

namespace SongCore::SongLoader {
    void CustomLevelPack::ctor(StringW packId, StringW packName, UnityEngine::Sprite* coverImage) {
        _ctor(packId, packName, packName, coverImage, coverImage, ArrayW<GlobalNamespace::BeatmapLevel*>::Empty(), GlobalNamespace::PlayerSensitivityFlag::Unknown);
    }

    CustomLevelPack* CustomLevelPack::New(std::string_view packId, std::string_view packName, UnityEngine::Sprite* coverImage) {
        return CustomLevelPack::New_ctor(packId, packName, coverImage);
    }

    void CustomLevelPack::SortLevels() {
        SortLevels([](auto a, auto b) -> bool { return static_cast<std::u16string_view>(a->songName) < static_cast<std::u16string_view>(b->songName); });
    }

    void CustomLevelPack::SortLevels(WeakSortingFunc sortingFunc) {
        std::stable_sort(beatmapLevels.begin(), beatmapLevels.end(), sortingFunc);
    }

    void CustomLevelPack::SetLevels(std::span<CustomBeatmapLevel* const> levels) {
        beatmapLevels = ArrayW<GlobalNamespace::BeatmapLevel*>(levels.size());
        std::copy(levels.begin(), levels.end(), beatmapLevels.begin());
    }

    void CustomLevelPack::SetLevels(std::span<GlobalNamespace::BeatmapLevel* const> levels) {
        beatmapLevels = ArrayW<GlobalNamespace::BeatmapLevel*>(levels.size());
        std::copy(levels.begin(), levels.end(), beatmapLevels.begin());
    }
}
