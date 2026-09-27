#include "Utils/OggVorbis.hpp"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <span>

#include "logging.hpp"
#include "Utils/File.hpp"

namespace SongCore::Utils {
    std::expected<OggPageHeader, ScanError> OggPageHeader::parse(std::span<std::byte> buffer) {

        if (buffer.size() < header_byte_size) return std::unexpected(ScanError::InvalidOggPageMagic);

        if (std::memcmp(buffer.data(), "OggS", 4) != 0) {
            return std::unexpected(ScanError::InvalidOggPageMagic);
        }

        uint8_t segments = static_cast<uint8_t>(buffer[26]);

        if (buffer.size() < header_byte_size + segments) {
            return std::unexpected(ScanError::InvalidOggPageMagic);
        }

        auto buff_begin_acc = buffer.begin() + header_byte_size;

        size_t body_size = std::accumulate(
            buff_begin_acc,
            buff_begin_acc + segments,
            size_t{0},
            [](size_t total, std::byte value) {
                return total + static_cast<size_t>(std::to_integer<uint8_t>(value));
            });

        return OggPageHeader{
            .header_type = {static_cast<uint8_t>(buffer[5])},
            .granule_position = *(int64_t*)(buffer.data() + 6),
            .serial_number = *(uint32_t*)(buffer.data() + 14),
            .page_sequence_number = *(uint32_t*)(buffer.data() + 18),
            .checksum = 0,
            .page_segments = segments,
            .header_size = header_byte_size + segments,
            .body_size = body_size
        };
    }

    std::expected<VorbisIdHeader, ScanError> VorbisIdHeader::parse(std::span<const std::byte> payload) {
        // Minimum identification packet is 30 bytes
        if (payload.size() < 30) return std::unexpected(ScanError::InvalidIdentificationHeader);
        
        // Packet starts with "\x01vorbis"
        const std::array<std::byte, 7> vorbis_magic = {
            std::byte{0x01}, std::byte{'v'}, std::byte{'o'}, std::byte{'r'},
            std::byte{'b'}, std::byte{'i'}, std::byte{'s'}
        };

        if (std::memcmp(payload.data(), vorbis_magic.data(), vorbis_magic.size()) != 0) {
            return std::unexpected(ScanError::NotVorbisStream);
        }

        return VorbisIdHeader{
            .audio_channels = *(uint8_t*)(payload.data() + 11),
            .audio_sample_rate = *(uint32_t*)(payload.data() + 12),
        };
    }

    std::expected<float, ScanError> GetLengthFromOggVorbis(std::filesystem::path path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return std::unexpected(ScanError::FileNotFound);

        const std::streamsize file_size = file.tellg();
        if (file_size < 27) return std::unexpected(ScanError::InvalidOggPageMagic);

        file.seekg(0, std::ios::beg);
        std::byte start_buf[4096];
        file.read(reinterpret_cast<char*>(&start_buf), std::min<size_t>(file_size, 4096));
        int skip = 1;
        for (auto i = 0; i < 4096 - OggPageHeader::header_byte_size; i += skip)
        {

            auto first_header = OggPageHeader::parse(std::span(start_buf).subspan(i));
            if (!first_header) return std::unexpected(first_header.error());

            // Validate BOS (Beginning of Stream) flag
            if (!(first_header->header_type.header_type.is_first_page_of_logical_bitstream)) {
                return std::unexpected(ScanError::InvalidOggPageMagic);
            }

            const uint32_t target_serial = first_header->serial_number;
            const auto payload_span = std::span(start_buf).subspan(
                i + first_header->header_size
            );

            auto vorbis_info = VorbisIdHeader::parse(payload_span);
            if (!vorbis_info) {
                if (i == 4095 - OggPageHeader::header_byte_size)
                    return std::unexpected(vorbis_info.error());
                else{
                    skip = first_header->header_size + first_header->body_size;
                    continue;
                }
            }

            // An Ogg page maximum theoretical size is ~65KB (27 + 255 + (255*255))
            // Reading the last 64-128KB is virtually guaranteed to catch the last page.
            constexpr size_t tail_chunk_size = 65536 * 2;
            const size_t bytes_to_read = std::min<size_t>(file_size, tail_chunk_size);
            
            file.seekg(-bytes_to_read, std::ios::end);
            std::vector<std::byte> tail_buf(bytes_to_read);
            file.read(reinterpret_cast<char*>(tail_buf.data()), bytes_to_read);

            // Scan backwards for 'OggS'
            int64_t last_granule_pos = -1;
            for (size_t i = tail_buf.size() - OggPageHeader::header_byte_size; i > 0; --i) {
                auto page = OggPageHeader::parse(std::span(tail_buf).subspan(i));

                if (page && page->serial_number == target_serial && (page->granule_position != -1 || page->header_type.header_type.is_last_page_of_logical_bitstream)) {
                        last_granule_pos = page->granule_position;
                        break;
                }
            }

            if (last_granule_pos == -1) {
                return std::unexpected(ScanError::StreamNotFoundAtEnd);
            }
            return static_cast<float>(static_cast<double>(last_granule_pos) / static_cast<double>(vorbis_info->audio_sample_rate));
        }
        return std::unexpected(ScanError::SeekFailed);
    }
}
