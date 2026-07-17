#include "Utils/WavRiff.hpp"
#include "song_load_rs.h"

namespace SongCore::Utils {
    float GetLengthFromWavRiff(std::filesystem::path const& path) {
        return song_core_get_audio_length_secs_from_path(path.string().c_str());
    }
}
