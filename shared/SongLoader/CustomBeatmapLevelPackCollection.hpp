#pragma once

#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/BeatmapLevelPackCollection.hpp"
#include "CustomLevelPack.hpp"

DECLARE_CLASS_CODEGEN(SongCore::SongLoader, CustomBeatmapLevelPackCollection, GlobalNamespace::BeatmapLevelPackCollection,
    DECLARE_CTOR(ctor);
    public:
        /// @brief clears the internal levelpack array
        void ClearPacks();

        /// @brief adds a custom levelpack to the internal array
        void AddPack(CustomLevelPack* levelPack, bool addIfEmpty = false);

        static CustomBeatmapLevelPackCollection* New();
)
