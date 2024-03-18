#include "SongLoader/CustomBeatmapLevelPackCollection.hpp"
#include "SongLoader/CustomLevelPack.hpp"

#include "GlobalNamespace/IBeatmapLevelCollection.hpp"
#include "System/Collections/Generic/IReadOnlyList_1.hpp"
#include "System/Collections/Generic/IReadOnlyCollection_1.hpp"

DEFINE_TYPE(SongCore::SongLoader, CustomBeatmapLevelPackCollection);

namespace SongCore::SongLoader {
    void CustomBeatmapLevelPackCollection::ctor() {
        _ctor(ArrayW<GlobalNamespace::IBeatmapLevelPack*>::Empty());
    }

    CustomBeatmapLevelPackCollection* CustomBeatmapLevelPackCollection::New() {
        return CustomBeatmapLevelPackCollection::New_ctor();
    }

    void CustomBeatmapLevelPackCollection::ClearPacks() {
        _beatmapLevelPacks = ArrayW<GlobalNamespace::IBeatmapLevelPack*>::Empty();
    }

    void CustomBeatmapLevelPackCollection::AddPack(CustomLevelPack* levelPack, bool addIfEmpty) {
        // if we don't want to force the add, and the pack has 0 levels, don't add it
        if (!addIfEmpty) { // dumb check for the levels because the interface method call can fail somehow
            auto beatmapLevels = levelPack->beatmapLevelCollection->beatmapLevels;
            auto get_Count_minfo = il2cpp_utils::FindMethod(levelPack->beatmapLevelCollection->beatmapLevels, "get_Count");
            if (get_Count_minfo) {
                auto count = il2cpp_utils::RunMethodRethrow<int32_t, false>(beatmapLevels, get_Count_minfo);
                if (count == 0) return;
            }
        }

        auto newArray = ArrayW<GlobalNamespace::IBeatmapLevelPack*>(_beatmapLevelPacks.size() + 1);
        std::copy_n(_beatmapLevelPacks->begin(), _beatmapLevelPacks.size(), newArray->begin());
        newArray[_beatmapLevelPacks.size()] = levelPack->i___GlobalNamespace__IBeatmapLevelPack();

        _beatmapLevelPacks = newArray;
    }
}
