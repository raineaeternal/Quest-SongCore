#include "SongLoader/SongCoreCustomBeatmapLevelPackCollection.hpp"
#include "SongLoader/SongCoreCustomLevelPack.hpp"

DEFINE_TYPE(SongCore::SongLoader, SongCoreCustomBeatmapLevelPackCollection);

namespace SongCore::SongLoader {
    void SongCoreCustomBeatmapLevelPackCollection::ctor() {
        _ctor(ArrayW<GlobalNamespace::IBeatmapLevelPack*>::Empty());
    }

    SongCoreCustomBeatmapLevelPackCollection* SongCoreCustomBeatmapLevelPackCollection::New() {
        return SongCoreCustomBeatmapLevelPackCollection::New_ctor();
    }

    void SongCoreCustomBeatmapLevelPackCollection::ClearPacks() {
        _beatmapLevelPacks = ArrayW<GlobalNamespace::IBeatmapLevelPack*>::Empty();
    }

    void SongCoreCustomBeatmapLevelPackCollection::AddPack(SongCoreCustomLevelPack* levelPack) {
        auto newArray = ArrayW<GlobalNamespace::IBeatmapLevelPack*>(_beatmapLevelPacks.size() + 1);
        std::copy_n(_beatmapLevelPacks->begin(), _beatmapLevelPacks.size(), newArray->begin());
        newArray[_beatmapLevelPacks.size()] = levelPack->i___GlobalNamespace__IBeatmapLevelPack();

        _beatmapLevelPacks = newArray;
    }
}
