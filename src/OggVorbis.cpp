#include "OggVorbis.hpp"
#include <cstddef>
#include <fstream>
#include <stdint.h>
#include "File.hpp"

#define OFFSET (8 + 2 + 4)

const char VORBIS[] = { 0x76, 0x6F, 0x72, 0x62, 0x69, 0x73 };
const char OGG[] = { 0x4F, 0x67, 0x67, 0x53, 0x00, 0x04 };

float GetLengthFromOggVorbis(std::string_view path) {
    std::ifstream reader(path, std::ios::in | std::ios::binary);

    auto findBytes = [&reader](std::vector<uint8_t>& bytes, int searchLength) -> bool {
        for (int i = 0; i < searchLength; i++) {
            uint8_t b;
            reader.read((char *)&b, sizeof(uint8_t));

            if (b != bytes[0]) {
                continue;
            }

            uint8_t by[bytes.size()];
            reader.read((char *)by, bytes.size() * sizeof(uint8_t) - 1);

            if (by[0] == bytes[1] 
                && by[1] == bytes[2] 
                && by[2] == bytes[3] 
                && by[3] == bytes[4] 
                && (by[4] & bytes[5]) == bytes[5]) {
                return true;
            }

            auto itr = std::find_if(bytes.begin(), bytes.end(), [](auto iter) {
                return iter->value;
            });
            int idx = -1;
        }
    };
}