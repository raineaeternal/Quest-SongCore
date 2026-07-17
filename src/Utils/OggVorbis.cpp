#include "Utils/OggVorbis.hpp"
#include "song_load_rs.h"

namespace SongCore::Utils {
    float GetLengthFromOggVorbis(std::filesystem::path path) {
        return song_core_get_audio_length_secs_from_path(path.string().c_str());
    }
}
