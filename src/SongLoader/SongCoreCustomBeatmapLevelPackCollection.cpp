#include "SongLoader/SongCoreCustomBeatmapLevelPackCollection.hpp"
#include "SongLoader/SongCoreCustomLevelPack.hpp"

#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Collections/Generic/IReadOnlyCollection_1.hpp"

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

    void SongCoreCustomBeatmapLevelPackCollection::AddPack(SongCoreCustomLevelPack* levelPack, bool addIfEmpty) {
        // if we don't want to force the add, and the pack has 0 levels, don't add it
        if (!addIfEmpty && levelPack->beatmapLevelCollection->beatmapLevels->i___System__Collections__Generic__IReadOnlyCollection_1_T_()->Count == 0) return;

        auto newArray = ArrayW<GlobalNamespace::IBeatmapLevelPack*>(_beatmapLevelPacks.size() + 1);
        std::copy_n(_beatmapLevelPacks->begin(), _beatmapLevelPacks.size(), newArray->begin());
        newArray[_beatmapLevelPacks.size()] = levelPack->i___GlobalNamespace__IBeatmapLevelPack();

        _beatmapLevelPacks = newArray;
    }
}
