#include "SongLoader/SongCoreCustomLevelPack.hpp"
#include "SongLoader/SongCoreCustomBeatmapLevelCollection.hpp"

DEFINE_TYPE(SongCore::SongLoader, SongCoreCustomLevelPack);

namespace SongCore::SongLoader {
    void SongCoreCustomLevelPack::ctor(StringW packId, StringW packName, UnityEngine::Sprite* coverImage) {
        _ctor(packId, packName, packName, coverImage, coverImage, SongCoreCustomBeatmapLevelCollection::New_ctor(), GlobalNamespace::PlayerSensitivityFlag::Unknown);
    }

    SongCoreCustomLevelPack* SongCoreCustomLevelPack::New(std::string_view packId, std::string_view packName, UnityEngine::Sprite* coverImage) {
        return SongCoreCustomLevelPack::New_ctor(packId, packName, coverImage);
    }

    void SongCoreCustomLevelPack::SortLevels() {

    }
}
