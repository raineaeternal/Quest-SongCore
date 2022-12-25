#pragma once

#include <string>

#include "System/Collections/Generic/Dictionary_2.hpp"

#include "custom-types/shared/macros.hpp"
#include "UnityEngine/MonoBehaviour.hpp"

#include "GlobalNamespace/BeatmapDataLoader.hpp"
#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp"

namespace SongCore::SongLoader {
    using Dictionary = ::System::Collections::Generic::Dictionary_2<StringW, ::GlobalNamespace::CustomPreviewBeatmapLevel*>*

public:
    std::string SongHelper::GetSongPath(std::string path);
    int SongHelper::GetSongCount(std::string path);
}

DECLARE_CLASS_CODEGEN(SongCore, SongLoader, UnityEngine::MonoBehaviour,
    DECLARE_INSTANCE_FIELD(Dictionary, CustomLevels);
    DECLARE_INSTANCE_FIELD(Dictionary, CustomWIPLevels);

    DECLARE_INSTANCE_FIELD(GlobalNamespace::BeatmapDataLoader*, BeatmapDataLoader);

)
