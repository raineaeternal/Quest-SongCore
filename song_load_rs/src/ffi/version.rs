use std::{ffi::CStr, path::Path};

use semver::Version;

use crate::version;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CVersion {
    pub major: u64,
    pub minor: u64,
    pub patch: u64,
}

impl From<Version> for CVersion {
    fn from(version: Version) -> Self {
        CVersion {
            major: version.major,
            minor: version.minor,
            patch: version.patch,
        }
    }
}

impl From<CVersion> for Version {
    fn from(c_version: CVersion) -> Self {
        Version {
            major: c_version.major,
            minor: c_version.minor,
            patch: c_version.patch,
            pre: semver::Prerelease::EMPTY,
            build: semver::BuildMetadata::EMPTY,
        }
    }
}

/// Gets the version from a file at the given path.
/// # Parameters
/// - `c_path`: A pointer to a null-terminated C string representing the file path.
/// # Safety
/// The `c_path` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_get_version_from_path(
    c_path: *const std::os::raw::c_char,
) -> CVersion {
    if c_path.is_null() {
        panic!("Path is null");
    }
    let path = unsafe { CStr::from_ptr(c_path) }
        .to_str()
        .expect("Failed to convert path to str");

    let version = version::version_from_file_path(Path::new(path)).unwrap_or(version::NO_VERSION);

    CVersion::from(version)
}

/// Gets the version from a given version string.
/// # Parameters
/// - `c_version_str`: A pointer to a null-terminated C string representing the version string.
/// # Safety
/// The `c_version_str` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_loader_get_version_from_string(
    c_version_str: *const std::os::raw::c_char,
) -> CVersion {
    if c_version_str.is_null() {
        panic!("Version string is null");
    }
    let version_str = unsafe { CStr::from_ptr(c_version_str) }
        .to_str()
        .expect("Failed to convert version string to str");

    let version = version::get_version(version_str).unwrap_or(version::NO_VERSION);

    CVersion::from(version)
}
