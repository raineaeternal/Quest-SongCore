#pragma once

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include <string>

DECLARE_CLASS_CODEGEN(SongCore::SongLoader, SongCoreCustomLevelPack, GlobalNamespace::CustomBeatmapLevelPack,
    DECLARE_CTOR(ctor, StringW packId, StringW packName, UnityEngine::Sprite* coverImage);

    public:
        static SongCoreCustomLevelPack* New(std::string_view packId, std::string_view packName, UnityEngine::Sprite* coverImage = nullptr);

        /// @brief sorts the levels in the collection, based on `a->songName < b->songName`
        void SortLevels();

        /// @brief sorting function that returns `a < b`
        using WeakSortingFunc = std::function<bool(GlobalNamespace::CustomPreviewBeatmapLevel*, GlobalNamespace::CustomPreviewBeatmapLevel*)>;

        /// @brief sorts the levels in the collection based on the inputted sort function. std::stable_sort is used
        void SortLevels(WeakSortingFunc sortingFunc);

        /// @brief sets the levels in the collection based on the inputted span
        void SetLevels(std::span<GlobalNamespace::CustomPreviewBeatmapLevel* const> levels);
)
