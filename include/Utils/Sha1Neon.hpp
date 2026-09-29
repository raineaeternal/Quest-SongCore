#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <arm_neon.h>

namespace SongCore::Utils {
    class SHA1_NEON {
    public:
        SHA1_NEON() { reset(); }

        void reset();

        void update(const uint8_t* data, size_t length);

        // Convenience overload for std::string
        void update(const std::string& str) { update(reinterpret_cast<const uint8_t*>(str.data()), str.size()); }

        std::string finalize();

    private:
        uint32x4_t state0;
        uint32_t state1;
        uint32x4_t magic0, magic1, magic2, magic3;

        uint8_t buffer[64];
        size_t buffer_len;
        uint64_t total_bytes;

        void process_block(const uint8_t* block);
    };
}