use std::{
    cell::RefCell,
    io::{self, Read},
    path::{Path, PathBuf},
};

use bytes::Bytes;
use zip::ZipArchive;

use crate::models::InfoDat;

/// Represents a beatmap, which can be either a zip archive or a directory.
///
/// TODO: Other sources?
pub enum Beatmap {
    //TODO: Can we use ZipArchive<Cursor<Bytes>> directly?
    // I would rather not make every read op &mut
    Zip(RefCell<ZipArchive<std::io::Cursor<Bytes>>>),
    Directory(PathBuf),
}

impl Beatmap {
    pub fn get_file_bytes(&self, file_path: &Path) -> io::Result<Bytes> {
        match self {
            Beatmap::Zip(zip_file) => {
                let mut zip_file_ref = zip_file.borrow_mut();

                let mut file = zip_file_ref
                    .by_name(file_path.to_str().unwrap())
                    .map_err(|e| io::Error::new(io::ErrorKind::NotFound, e))?;
                let mut buffer = Vec::with_capacity(file.size() as usize);
                file.read_to_end(&mut buffer)?;
                Ok(Bytes::from(buffer))
            }
            Beatmap::Directory(dir_path) => {
                let full_path = dir_path.join(file_path);
                let bytes = std::fs::read(full_path)?;
                Ok(Bytes::from(bytes))
            }
        }
    }

    /// Creates a Beatmap from the given path, which can be either a zip file or a directory.
    /// Returns an error if the path does not exist or is not accessible.
    ///
    pub fn from_path(path: &Path) -> io::Result<Beatmap> {
        if !path.exists() {
            return Err(io::Error::new(
                io::ErrorKind::NotFound,
                "Path does not exist",
            ));
        }

        if path.is_file() {
            let zip_bytes = std::fs::read(path)?;
            let cursor = std::io::Cursor::new(bytes::Bytes::from(zip_bytes));
            let archive = zip::ZipArchive::new(cursor)?;

            return Ok(Beatmap::Zip(archive.into()));
        }
        Ok(Beatmap::Directory(path.to_path_buf()))
    }

    pub fn get_info_dat_bytes(&self) -> io::Result<Bytes> {
        self.get_file_bytes(Path::new("Info.dat"))
            .or_else(|_| self.get_file_bytes(Path::new("info.dat")))
    }

    pub fn get_info_dat(&self) -> io::Result<InfoDat> {
        let info_bytes = self.get_info_dat_bytes()?;
        let info_vec = info_bytes.to_vec();

        let info_contents = String::from_utf8(info_vec)
            .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;
        let info_dat: InfoDat = serde_json::from_str(&info_contents)
            .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

        Ok(info_dat)
    }
}
