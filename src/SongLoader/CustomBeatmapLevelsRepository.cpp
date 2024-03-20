#include "SongLoader/CustomBeatmapLevelsRepository.hpp"
#include "logging.hpp"

DEFINE_TYPE(SongCore::SongLoader, CustomBeatmapLevelsRepository);

namespace SongCore::SongLoader {
    void CustomBeatmapLevelsRepository::ctor() {
        INVOKE_CTOR();
        _levelPacks = ListW<GlobalNamespace::BeatmapLevelPack*>::New();
        _ctor(_levelPacks->i___System__Collections__Generic__IReadOnlyList_1_T_());
    }

    void CustomBeatmapLevelsRepository::AddLevelPack(GlobalNamespace::BeatmapLevelPack* pack) {
        auto existing = GetBeatmapLevelPackByPackId(pack->packID);
        if (!existing) {
            _levelPacks->Add(pack);
        } else {
            WARNING("A pack with id {} was already added, not adding again!", pack->packID);
        }
    }

    void CustomBeatmapLevelsRepository::RemoveLevelPack(GlobalNamespace::BeatmapLevelPack* pack) {
        auto existing = GetBeatmapLevelPackByPackId(pack->packID);
        if (existing) {
            _levelPacks->Remove(existing);
        } else {
            WARNING("A pack with id {} was not added, not removing!", pack->packID);
        }
    }

    void CustomBeatmapLevelsRepository::ClearLevelPacks() {
        _levelPacks.clear();
    }
}
