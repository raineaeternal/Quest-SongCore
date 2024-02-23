#include "SongLoader/SongLoader.hpp"
#include "logging.hpp"

#include "File.hpp"

#include <filesystem>
#include <string.h>

DEFINE_TYPE(SongCore::SongLoader, RuntimeSongLoader);

namespace SongCore::SongLoader {
    __declspec(property(get = get_songPath, put = set_songPath)) std::filesystem::path songPath;
    __declspec(property(get = get_wipSongPath)) std::filesystem::path wipSongPath;
    __declspec(property(get = get_songs)) ListW<GlobalNamespace::BeatmapData*> _songs[] = {};
    __declspec(property(get = get_songCount)) int songCount;
    __declspec(property(get = get_customLevels)) SongLoader::SongDict* CustomLevels;
    __declspec(property(get = get_customWIPLevels)) SongLoader::SongDict* CustomWIPLevels;

    void RuntimeSongLoader::Awake() {
        songPath = "/sdcard/ModData/com.beatgames.beatsaber/Mods/SongLoader/CustomLevels";
        RuntimeSongLoader::LoadSongs(songPath.c_str());
    }

    void RuntimeSongLoader::Update() {

    }

    void RuntimeSongLoader::LoadCustomPreviewLevel(std::string_view path) {
        
    }

    void RuntimeSongLoader::LoadSongs(std::string_view path) {
        auto dirs = Utils::GetFolders(path);
        for (auto entry : dirs) {
            DEBUG("Found directory in {} with name {}", path, entry);
        }
        DEBUG("Found {} directories in path {}", dirs.size(), path);
    }
}
