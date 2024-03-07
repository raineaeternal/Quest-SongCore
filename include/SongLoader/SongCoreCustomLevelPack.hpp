#pragma once

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp"
#include <string>

DECLARE_CLASS_CODEGEN(SongCore::SongLoader, SongCoreCustomLevelPack, GlobalNamespace::CustomBeatmapLevelPack,
    DECLARE_CTOR(ctor, StringW packId, StringW packName, UnityEngine::Sprite* coverImage);

    public:
        static SongCoreCustomLevelPack* New(std::string_view packId, std::string_view packName, UnityEngine::Sprite* coverImage = nullptr);

        /// @brief sorts the levels in the collection
        void SortLevels();

        /* TODO: sort method to sort based on an input std::function? */
)
