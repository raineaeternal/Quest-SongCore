use std::{
    fs::File,
    io::{Read, Seek, SeekFrom},
    path::Path, sync::LazyLock,
};

use regex::Regex;
use semver::{BuildMetadata, Prerelease, Version};

/// A constant representing no version (0.0.0).
pub const NO_VERSION: Version = Version {
    major: 0,
    minor: 0,
    patch: 0,
    pre: Prerelease::EMPTY,
    build: BuildMetadata::EMPTY,
};

/// Attempts to extract the version from the given data string.
/// Looks for a JSON field named "version" or "_version" and parses its value.
/// Returns None if no version field is found or if parsing fails.
#[inline]
pub fn get_version(data: &[u8]) -> Option<Version> {
    if data.is_empty() {
        return None;
    }

    static RE: LazyLock<Regex> = LazyLock::new(|| Regex::new(r#""_?version"\s*:\s*"([0-9]+\.[0-9]+(?:\.[0-9]+)?)""#).unwrap());

    // Only examine the first 50 bytes for performance
    let truncated = &data[..data.len().min(50)];

    // Convert to &str for regex processing
    let truncated = std::str::from_utf8(truncated).ok()?;

    let captures = RE.captures(truncated)?;
    let m = captures.get(1)?;

    let ver_str = m.as_str();
    Version::parse(ver_str).ok()
}

/// Attempts to read the version from a file at the given path.
/// Reads the first up to 50 bytes of the file and looks for a version string.
/// Returns None if the file does not exist or cannot be read.
pub fn version_from_file_path(path: &Path) -> Option<Version> {
    if !path.exists() {
        return None;
    }

    let mut file = match File::open(path) {
        Ok(f) => f,
        Err(_) => return None,
    };

    let size = file.seek(SeekFrom::End(0)).unwrap_or(0) as usize;
    let to_read = std::cmp::min(size, 50);

    if file.seek(SeekFrom::Start(0)).is_err() {
        return Some(NO_VERSION);
    }

    let mut buf = vec![0u8; to_read];
    let n = file.read(&mut buf).ok()?;


    get_version(&buf[..n])
}
