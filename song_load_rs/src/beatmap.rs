use std::{
    cell::RefCell,
    fmt::Display,
    io::{self, Read},
    path::{Path, PathBuf},
};

use bytes::Bytes;
use zip::ZipArchive;

use crate::{info_dat::InfoDat, version};

/// Represents a beatmap, which can be either a zip archive or a directory.
///
/// TODO: Other sources?
#[derive(Debug, Clone)]
pub enum BeatmapSource {
    //TODO: Can we use ZipArchive<Cursor<Bytes>> directly?
    // I would rather not make every read op &mut
    Zip(PathBuf, RefCell<ZipArchive<std::io::Cursor<Bytes>>>),
    Directory(PathBuf),
}

impl BeatmapSource {
    /// Returns the real path of the beatmap source.
    /// For Zip sources, this is the path to the zip file.
    /// For Directory sources, this is the path to the directory.
    ///
    pub fn get_real_path(&self) -> &Path {
        match self {
            BeatmapSource::Zip(path, _) => path,
            BeatmapSource::Directory(path) => path,
        }
    }

    pub fn has_file(&self, file_path: &Path) -> bool {
        match self {
            BeatmapSource::Zip(_, zip_file) => {
                let mut zip_file_ref = zip_file.borrow_mut();
                zip_file_ref.by_name(file_path.to_str().unwrap()).is_ok()
            }
            BeatmapSource::Directory(dir_path) => dir_path.join(file_path).exists(),
        }
    }

    pub fn get_file_bytes<P>(&self, file_path: P) -> io::Result<Bytes>
    where
        P: AsRef<Path>,
    {
        match self {
            BeatmapSource::Zip(_, zip_file) => {
                let mut zip_file_ref = zip_file.borrow_mut();

                let mut file = zip_file_ref
                    .by_name(&file_path.as_ref().to_string_lossy())
                    .map_err(|e| io::Error::new(io::ErrorKind::NotFound, e))?;

                let mut buffer = Vec::with_capacity(file.size() as usize);
                file.read_to_end(&mut buffer)?;
                Ok(Bytes::from(buffer))
            }
            BeatmapSource::Directory(dir_path) => {
                let full_path = dir_path.join(file_path);
                let bytes = std::fs::read(full_path)?;
                Ok(Bytes::from(bytes))
            }
        }
    }

    /// Creates a Beatmap from the given path, which can be either a zip file or a directory.
    /// Returns an error if the path does not exist or is not accessible.
    ///
    pub fn from_path(path: PathBuf) -> io::Result<BeatmapSource> {
        if !path.exists() {
            return Err(io::Error::new(
                io::ErrorKind::NotFound,
                "Path does not exist",
            ));
        }

        if path.is_file() {
            let zip_bytes = std::fs::read(&path)?;
            let cursor = std::io::Cursor::new(bytes::Bytes::from(zip_bytes));
            let archive = zip::ZipArchive::new(cursor)?;

            return Ok(BeatmapSource::Zip(path, archive.into()));
        }
        Ok(BeatmapSource::Directory(path.to_path_buf()))
    }

    /// Reads and returns the Info.dat or info.dat file bytes from the beatmap source.
    pub fn get_info_dat_bytes(&self) -> io::Result<Bytes> {
        self.get_file_bytes(Path::new("Info.dat"))
            .or_else(|_| self.get_file_bytes(Path::new("info.dat")))
    }

    /// Parses and returns the Info.dat data from the beatmap source.
    /// Automatically detects the version and deserializes into the appropriate struct.
    #[inline]
    pub fn get_info_dat(&self) -> io::Result<(Bytes, InfoDat)> {
        let info_bytes = self.get_info_dat_bytes()?;
        let version = version::get_version(&info_bytes).unwrap_or(version::NO_VERSION);

        match version {
            v if v == version::NO_VERSION || v.major == 2 => {
                let info_dat: crate::models::info_dat::v2::StandardLevelInfoSaveDataV2 =
                    serde_json::from_slice(&info_bytes)
                        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;
                Ok((info_bytes, InfoDat::V2(info_dat)))
            }
            v if v.major == 4 => {
                let info_dat: crate::models::info_dat::v4::BeatmapLevelSaveDataV4 =
                    serde_json::from_slice(&info_bytes)
                        .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;
                Ok((info_bytes, InfoDat::V4(info_dat)))
            }
            _ => Err(io::Error::new(
                io::ErrorKind::InvalidData,
                "Unsupported Info.dat version",
            )),
        }
    }

    /// Loads the beatmap level from the beatmap source.
    /// Uses the appropriate loader based on the Info.dat version.
    ///
    /// `wip` indicates whether to load WIP data.
    /// `song_cache` is used to get cached song data, or build the cache if not present.
    ///
    #[cfg(feature = "level-loading")]
    pub fn load_level<C>(
        &self,
        wip: bool,
        song_cache: Option<&crate::utils::SongCoreLock<C>>,
    ) -> Result<Option<CustomBeatmapLevel>, CustomLevelLoaderError>
    where
        C: SongCache + ?Sized,
    {
        level_loader::load_level_from_path(self, wip, song_cache)
    }
}

impl Display for BeatmapSource {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            BeatmapSource::Zip(path, _) => write!(f, "BeatmapSource::Zip({})", path.display()),
            BeatmapSource::Directory(path) => {
                write!(f, "BeatmapSource::Directory({})", path.display())
            }
        }
    }
}
