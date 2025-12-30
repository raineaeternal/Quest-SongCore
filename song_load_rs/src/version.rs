use std::{
    fs::File,
    io::{Read, Seek, SeekFrom},
    path::Path,
};

use regex::Regex;
use semver::{BuildMetadata, Prerelease, Version};

const NO_VERSION: Version = Version {
    major: 0,
    minor: 0,
    patch: 0,
    pre: Prerelease::EMPTY,
    build: BuildMetadata::EMPTY,
};

pub fn get_version(data: &str) -> Option<Version> {
    if data.is_empty() {
        return None;
    }

    let truncated = &data[..data.len().min(50)];
    let re = Regex::new(r#""_?version"\s*:\s*"([0-9]+\.[0-9]+(?:\.[0-9]+)?)""#).unwrap();

    let caps = re.captures(truncated)?;
    let m = caps.get(1)?;

    let ver_str = m.as_str();
    Version::parse(ver_str).ok()
}

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

    let start_of_file = String::from_utf8_lossy(&buf[..n]).into_owned();
    get_version(&start_of_file)
}
