#pragma once

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <expected>
#include <fstream>
#include <numeric>
#include <span>
#include <vector>

namespace SongCore::Utils {
    enum class ScanError {
        FileNotFound,
        InvalidOggPageMagic,
        NotVorbisStream,
        InvalidIdentificationHeader,
        StreamNotFoundAtEnd,
        SeekFailed
    };

    union OggHeaderType{
        uint8_t header_type_byte;
        struct {
            bool is_continued_packet : 1;
            bool is_first_page_of_logical_bitstream : 1;
            bool is_last_page_of_logical_bitstream : 1;
        } header_type;
    };

    struct OggPageHeader {
        OggHeaderType header_type;
        int64_t granule_position;
        uint32_t serial_number;
        uint32_t page_sequence_number;
        uint32_t checksum;
        uint8_t page_segments;
        size_t header_size;
        size_t body_size;
        std::vector<std::byte> payload;
        static constexpr uint32_t header_byte_size = 27;

        static std::expected<OggPageHeader, ScanError> parse(std::span<std::byte> buffer);
    };

    struct VorbisIdHeader {
        uint8_t audio_channels;
        uint32_t audio_sample_rate;

        static std::expected<VorbisIdHeader, ScanError> parse(std::span<const std::byte> payload);
    };

    std::expected<float, ScanError> GetLengthFromOggVorbis(std::filesystem::path path);
}
