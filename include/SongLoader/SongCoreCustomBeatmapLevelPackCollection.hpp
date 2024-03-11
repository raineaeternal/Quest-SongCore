#pragma once

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "SongCoreCustomLevelPack.hpp"

DECLARE_CLASS_CODEGEN(SongCore::SongLoader, SongCoreCustomBeatmapLevelPackCollection, GlobalNamespace::BeatmapLevelPackCollection,
    DECLARE_CTOR(ctor);
    public:
        /// @brief clears the internal levelpack array
        void ClearPacks();

        /// @brief adds a custom levelpack to the internal array
        void AddPack(SongCoreCustomLevelPack* levelPack);

        static SongCoreCustomBeatmapLevelPackCollection* New();
)
