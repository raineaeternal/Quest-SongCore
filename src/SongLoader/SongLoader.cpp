#include "SongLoader/SongLoader.hpp"
#include "main.hpp"
#include "shared/SongCore.hpp"
#include "logging.hpp"
#include "paper/shared/utfcpp/source/utf8.h"

#include "Utils/File.hpp"

#include <clocale>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <string.h>
#include <string>

DEFINE_TYPE(SongCore::SongLoader, RuntimeSongLoader);

namespace SongCore::SongLoader {
    void RuntimeSongLoader::Awake() {
        RuntimeSongLoader::LoadCustomLevels("/sdcard/ModData/com.beatgames.beatsaber/Mods/SongLoader/CustomLevels");
    }

    void RuntimeSongLoader::Update() {

    }

    void RuntimeSongLoader::LoadCustomLevels(std::string_view path) {
        auto dirs = Utils::GetFolders(path);
        for (auto folderPath : dirs) {
            DEBUG("Found directory in {} with name \"{}\"", path, folderPath.substr(folderPath.find_last_of("/") + 1));
            auto dataPath = folderPath + "/info.dat";

            if (!fileexists(dataPath)) {
                dataPath = folderPath + "/Info.dat";
            }
            auto text = Utils::ReadText16(dataPath);

            DEBUG("{}", dataPath);
            DEBUG("info.dat contents: {}", utf8::utf16to8(text));
        }

        DEBUG("Found {} directories in path {}", dirs.size(), path);
    }
}
