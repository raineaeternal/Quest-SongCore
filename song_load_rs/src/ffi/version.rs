use semver::Version;

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
