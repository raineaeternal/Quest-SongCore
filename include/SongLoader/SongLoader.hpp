#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <vector>

#include "SongCoreCustomLevelPack.hpp"

#include "System/Collections/Generic/Dictionary_2.hpp"
#include "System/Collections/Generic/List_1.hpp"

#include "custom-types/shared/macros.hpp"

#include "GlobalNamespace/CustomPreviewBeatmapLevel.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelCollection.hpp" 
#include "GlobalNamespace/CustomBeatmapLevelPack.hpp" 
#include "GlobalNamespace/StandardLevelInfoSaveData.hpp" 
#include "GlobalNamespace/EnvironmentInfoSO.hpp" 
#include "GlobalNamespace/BeatmapLevelData.hpp"
#include "GlobalNamespace/BeatmapDataLoader.hpp" 
#include "UnityEngine/MonoBehaviour.hpp" 
#include "Zenject/IInitializable.hpp"
#include "lapiz/shared/macros.hpp"

namespace SongCore::SongLoader {
    using SongDict = ::System::Collections::Generic::Dictionary_2<StringW, ::GlobalNamespace::CustomPreviewBeatmapLevel*>;
}


DECLARE_CLASS_CODEGEN(SongCore::SongLoader, RuntimeSongLoader, UnityEngine::MonoBehaviour, 
    DECLARE_INSTANCE_METHOD(void, Awake);
    DECLARE_INSTANCE_METHOD(void, Update);

    DECLARE_INSTANCE_FIELD_PRIVATE(int, _songCount);

    DECLARE_INSTANCE_FIELD_PRIVATE(SongDict*, _customLevels);
    DECLARE_INSTANCE_FIELD_PRIVATE(SongDict*, _customWIPLevels);

    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::BeatmapDataLoader*, _beatmapDataLoader);
    RuntimeSongLoader* _instance;

    std::filesystem::path _songPath, _wipSongPath;
    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> _loadedLevels;
    std::vector<GlobalNamespace::BeatmapLevelData*> _song;

public:
    RuntimeSongLoader* get_instance() { return _instance; };
    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> get_loadedLevels() { return _loadedLevels; };
    std::filesystem::path get_songPath() { return _songPath; };
    void set_songPath(std::string path);
    std::filesystem::path get_wipSongPath() { return _wipSongPath; };
    int get_songCount() { return _songCount; };
    SongDict* get_customLevels() { return _customLevels; };
    SongDict* get_customWIPLevels() { return _customWIPLevels; };

    void LoadCustomPreviewLevel(std::string_view path);
    void LoadSongs(std::string_view path);
    void CreateSongDirectoryIfNotExist();

)
