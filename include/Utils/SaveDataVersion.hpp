#pragma once

#include "System/Version.hpp"

namespace SongCore {
    #define VERSION_CMP_OP(op) bool operator op(Version const& rhs) const { return major op rhs.major && minor op rhs.minor && patch op rhs.patch; }
    struct Version {
        int major, minor, patch;
        constexpr Version(int major = 0, int minor = 0, int patch = 0) noexcept : major(major), minor(minor), patch(patch) {}
        explicit Version(System::Version* ver) : major(ver->Major), minor(ver->Minor), patch(ver->Build) {}

        operator System::Version*() const {
            return System::Version::New_ctor(major, minor, patch);
        }

        VERSION_CMP_OP(<)
        VERSION_CMP_OP(<=)
        VERSION_CMP_OP(==)
        VERSION_CMP_OP(>)
        VERSION_CMP_OP(>=)

        static Version noVersion;
    };

    /// @brief parses a version from the filepath that should be pointing to a json file
    Version VersionFromFilePath(std::filesystem::path const& filePath);
    /// @brief parses a version from the data
    Version VersionFromFileData(std::string const& data);
}
