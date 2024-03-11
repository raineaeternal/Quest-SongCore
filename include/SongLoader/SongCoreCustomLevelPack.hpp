#pragma once

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include <compare>
#include <string>

namespace SongCore::SongLoader {
    class RuntimeSongLoader;
}

DECLARE_CLASS_CODEGEN(SongCore::SongLoader, SongCoreCustomLevelPack, GlobalNamespace::CustomBeatmapLevelPack,
    DECLARE_CTOR(ctor, StringW packId, StringW packName, UnityEngine::Sprite* coverImage);

    public:
        static SongCoreCustomLevelPack* New(std::string_view packId, std::string_view packName, UnityEngine::Sprite* coverImage = nullptr);

        /// @brief sorts the levels in the collection
        void SortLevels();

        using WeakSortingFunc = std::function<bool(GlobalNamespace::CustomPreviewBeatmapLevel*, GlobalNamespace::CustomPreviewBeatmapLevel*)>;

        /// @brief sorts the levels in the collection based on the inputted sort function
        void SortLevels(WeakSortingFunc sortingFunc);
    private:
        friend class ::SongCore::SongLoader::RuntimeSongLoader;
        void SetLevels(std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> levels);
)
