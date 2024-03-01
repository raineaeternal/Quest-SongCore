#pragma once

#include <string>
#include <string_view>
#include <filesystem>
#include <vector>

#include "SongCoreCustomLevelPack.hpp"

#include "System/Collections/Concurrent/ConcurrentDictionary_2.hpp"
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
    using SongDict = ::System::Collections::Concurrent::ConcurrentDictionary_2<StringW, ::GlobalNamespace::CustomPreviewBeatmapLevel*>;
}


DECLARE_CLASS_CODEGEN(SongCore::SongLoader, RuntimeSongLoader, UnityEngine::MonoBehaviour, 
    DECLARE_INSTANCE_METHOD(void, Awake);
    DECLARE_INSTANCE_METHOD(void, Update);

    DECLARE_INSTANCE_FIELD_PRIVATE(int, _songCount);

    DECLARE_INSTANCE_FIELD_PRIVATE(SongDict*, _customLevels);
    DECLARE_INSTANCE_FIELD_PRIVATE(SongDict*, _customWIPLevels);

    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::BeatmapDataLoader*, _beatmapDataLoader);

    std::filesystem::path _songPath, _wipSongPath;
    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> _loadedLevels;
    std::vector<GlobalNamespace::BeatmapLevelData*> _song;

public:

    /// @brief Returns an instance of the songloader
    RuntimeSongLoader* get_instance() { return _instance; };
    RuntimeSongLoader* _instance = nullptr;

    /// @brief Returns the current song path.
    std::filesystem::path get_songPath() { return _songPath; };

    /// @brief brief description
    /// @param path Path to set the song directory to
    void set_songPath(std::string path);
    __declspec(property(get = get_songPath, put = set_songPath)) std::filesystem::path songPath;

    /// @brief Returns the current WIP song path.
    std::filesystem::path get_wipSongPath() { return _wipSongPath; };
    __declspec(property(get = get_wipSongPath)) std::filesystem::path wipSongPath;

    __declspec(property(get = get_songs)) ListW<GlobalNamespace::BeatmapData*> _songs[];

    /// @brief Returns the current song count
    int get_songCount() { return _songCount; };
    __declspec(property(get = get_songCount)) int songCount;

    /// @brief Returns a key value pair of songs
    SongDict* get_customLevels() { return _customLevels; };
    __declspec(property(get = get_customLevels)) SongLoader::SongDict* CustomLevels;

    /// @brief Returns a key value pair of the WIP songs
    SongDict* get_customWIPLevels() { return _customWIPLevels; };
    __declspec(property(get = get_customWIPLevels)) SongLoader::SongDict* CustomWIPLevels;

    /// @brief Returns a vector of currently loaded levels
    std::vector<GlobalNamespace::CustomPreviewBeatmapLevel*> get_loadedLevels() { return _loadedLevels; };

    /// @brief Loads songs at given path given
    /// @param path 
    void LoadCustomLevels(std::string_view path);

    /// @brief Creates the CustomLevels directory if it doesn't exist
    void CreateCustomLevelsDirectoryIfNotExist();

)
