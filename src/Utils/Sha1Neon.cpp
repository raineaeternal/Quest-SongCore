#include "Utils/Sha1Neon.hpp"
#include <fstream>

#define SHA1_ROUND(func, w, k)                                        \
    do {                                                              \
        uint32x4_t wk = vaddq_u32(w, k);                              \
        uint32_t next_state1 = vsha1h_u32(vgetq_lane_u32(state0, 0)); \
        state0 = func(state0, state1, wk);                            \
        state1 = next_state1;                                         \
    } while (0)


namespace SongCore::Utils {
    void SHA1_NEON::reset() {
        uint32_t init_state0[4] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
        state0 = vld1q_u32(init_state0);
        state1 = 0xC3D2E1F0;
        
        magic0 = vdupq_n_u32(0x5A827999);
        magic1 = vdupq_n_u32(0x6ED9EBA1);
        magic2 = vdupq_n_u32(0x8F1BBCDC);
        magic3 = vdupq_n_u32(0xCA62C1D6);

        total_bytes = 0;
        buffer_len = 0;
    }

    // Process a chunk of data (can be called repeatedly)
    void SHA1_NEON::update(const uint8_t* data, size_t length) {
        total_bytes += length;
        size_t offset = 0;

        // If buffer already contains bytes, fill it up to 64 bytes and process
        if (buffer_len > 0) {
            size_t to_fill = 64 - buffer_len;
            if (length >= to_fill) {
                std::memcpy(buffer + buffer_len, data, to_fill);
                process_block(buffer);
                offset += to_fill;
                buffer_len = 0;
            } else {
                std::memcpy(buffer + buffer_len, data, length);
                buffer_len += length;
                return;
            }
        }

        // Process full 64-byte blocks directly from the input pointer
        while (offset + 64 <= length) {
            process_block(data + offset);
            offset += 64;
        }

        // Store remaining bytes into the buffer to be used if update is called again or during finalize.
        if (offset < length) {
            buffer_len = length - offset;
            std::memcpy(buffer, data + offset, buffer_len);
        }
    }

    bool SHA1_NEON::updateFile(std::filesystem::path const& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;

        std::vector<uint8_t> chunk(64 * 1024);
        while (file) {
            file.read(reinterpret_cast<char*>(chunk.data()), chunk.size());
            auto read = file.gcount();
            if (read > 0) update(chunk.data(), static_cast<size_t>(read));
        }
        return file.eof();
    }

    // Finalize padding and return the hex string
    std::string SHA1_NEON::finalize() {
        uint64_t bit_len = total_bytes * 8;

        // Append 0x80 byte '1' bit
        buffer[buffer_len++] = 0x80;

        // If not enough room for the 8-byte length at buffer end, pad to end and process block
        if (buffer_len > 56) {
            std::memset(buffer + buffer_len, 0, 64 - buffer_len);
            process_block(buffer);
            buffer_len = 0;
        }

        // Pad with zeros up to 56 bytes
        std::memset(buffer + buffer_len, 0, 56 - buffer_len);

        // Append 64-bit length (big-endian)
        for (int i = 7; i >= 0; --i) {
            buffer[56 + (7 - i)] = static_cast<uint8_t>((bit_len >> (i * 8)) & 0xFF);
        }

        process_block(buffer);

        // Format to uppercase hex, matching the CryptoPP HexEncoder output used previously
        char hex[41];
        std::snprintf(hex, sizeof(hex), "%08X%08X%08X%08X%08X",
                    vgetq_lane_u32(state0, 0),
                    vgetq_lane_u32(state0, 1),
                    vgetq_lane_u32(state0, 2),
                    vgetq_lane_u32(state0, 3),
                    state1);
        return std::string(hex);
    }

    void SHA1_NEON::process_block(const uint8_t* block) {
        uint32x4_t state0_prev = state0;
        uint32_t state1_prev = state1;

        // Load 64 bytes and byte-swap to big-endian
        uint32x4_t w0 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(block + 0)));
        uint32x4_t w1 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(block + 16)));
        uint32x4_t w2 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(block + 32)));
        uint32x4_t w3 = vreinterpretq_u32_u8(vrev32q_u8(vld1q_u8(block + 48)));

        // Rounds 0-15
        SHA1_ROUND(vsha1cq_u32, w0, magic0);
        SHA1_ROUND(vsha1cq_u32, w1, magic0);
        SHA1_ROUND(vsha1cq_u32, w2, magic0);
        SHA1_ROUND(vsha1cq_u32, w3, magic0);

        // Rounds 16-19
        w0 = vsha1su1q_u32(vsha1su0q_u32(w0, w1, w2), w3);
        SHA1_ROUND(vsha1cq_u32, w0, magic0);

        // Rounds 20-35
        w1 = vsha1su1q_u32(vsha1su0q_u32(w1, w2, w3), w0);
        SHA1_ROUND(vsha1pq_u32, w1, magic1);
        w2 = vsha1su1q_u32(vsha1su0q_u32(w2, w3, w0), w1);
        SHA1_ROUND(vsha1pq_u32, w2, magic1);
        w3 = vsha1su1q_u32(vsha1su0q_u32(w3, w0, w1), w2);
        SHA1_ROUND(vsha1pq_u32, w3, magic1);
        w0 = vsha1su1q_u32(vsha1su0q_u32(w0, w1, w2), w3);
        SHA1_ROUND(vsha1pq_u32, w0, magic1);

        // Rounds 36-39
        w1 = vsha1su1q_u32(vsha1su0q_u32(w1, w2, w3), w0);
        SHA1_ROUND(vsha1pq_u32, w1, magic1);

        // Rounds 40-55
        w2 = vsha1su1q_u32(vsha1su0q_u32(w2, w3, w0), w1);
        SHA1_ROUND(vsha1mq_u32, w2, magic2);
        w3 = vsha1su1q_u32(vsha1su0q_u32(w3, w0, w1), w2);
        SHA1_ROUND(vsha1mq_u32, w3, magic2);
        w0 = vsha1su1q_u32(vsha1su0q_u32(w0, w1, w2), w3);
        SHA1_ROUND(vsha1mq_u32, w0, magic2);
        w1 = vsha1su1q_u32(vsha1su0q_u32(w1, w2, w3), w0);
        SHA1_ROUND(vsha1mq_u32, w1, magic2);

        // Rounds 56-59
        w2 = vsha1su1q_u32(vsha1su0q_u32(w2, w3, w0), w1);
        SHA1_ROUND(vsha1mq_u32, w2, magic2);

        // Rounds 60-75
        w3 = vsha1su1q_u32(vsha1su0q_u32(w3, w0, w1), w2);
        SHA1_ROUND(vsha1pq_u32, w3, magic3);
        w0 = vsha1su1q_u32(vsha1su0q_u32(w0, w1, w2), w3);
        SHA1_ROUND(vsha1pq_u32, w0, magic3);
        w1 = vsha1su1q_u32(vsha1su0q_u32(w1, w2, w3), w0);
        SHA1_ROUND(vsha1pq_u32, w1, magic3);
        w2 = vsha1su1q_u32(vsha1su0q_u32(w2, w3, w0), w1);
        SHA1_ROUND(vsha1pq_u32, w2, magic3);

        // Rounds 76-79
        w3 = vsha1su1q_u32(vsha1su0q_u32(w3, w0, w1), w2);
        SHA1_ROUND(vsha1pq_u32, w3, magic3);

        // Accumulate state0
        state0 = vaddq_u32(state0, state0_prev);
        state1 += state1_prev;
    }
}