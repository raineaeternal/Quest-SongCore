use std::{
    ffi::CStr,
    os::raw::c_char,
    path::{Path, PathBuf},
};

use crate::{
    beatmap::BeatmapSource,
    ffi::{
        cache::CSongCache,
        types::{ManagedArray, ManagedCString, OpaqueUserData},
    },
    loader::level_loader::{self, CustomBeatmapLevel},
    models::{
        beatmap::{
            BeatmapBasicData, FileSystemBeatmapLevelData, PlayerSensitivityFlag, PreviewMediaData,
        },
        common::Color,
        info_dat::ColorScheme,
    },
};

#[repr(C)]
pub struct CCustomBeatmapLevel {
    pub version: u32,
    pub has_precalculated_data: bool,
    pub level_id: ManagedCString,
    pub song_name: ManagedCString,
    pub song_sub_name: ManagedCString,
    pub song_author_name: ManagedCString,
    pub all_mappers: ManagedArray<ManagedCString>,
    pub all_lighters: ManagedArray<ManagedCString>,
    pub beats_per_minute: f32,
    pub integrated_lufs: f32,
    pub song_time_offset: f32,
    pub preview_start_time: f32,
    pub preview_duration: f32,
    pub song_duration: f32,
    pub content_rating: PlayerSensitivityFlag,
    pub preview_media_data: CPreviewMediaData,
    pub beatmap_basic_datas: ManagedArray<CBeatmapBasicDataTuple>,

    // pub custom_level_data: InfoDat,
    pub beatmap_level_data: CFileSystemBeatmapLevelData,
    pub custom_level_path: ManagedCString,
}
#[repr(C)]
pub struct CBeatmapBasicDataTuple {
    pub characteristic: ManagedCString,
    pub difficulty: ManagedCString,
    pub basic_data: CBeatmapBasicData,
}
#[repr(C)]
pub struct CBeatmapBasicData {
    pub note_jump_movement_speed: f32,
    pub note_jump_start_beat_offset: f32,
    pub environment: ManagedCString,
    pub color_scheme: CColorScheme,
    pub notes_count: u32,
    pub cuttable_objects_count: u32,
    pub obstacles_count: u32,
    pub bombs_count: u32,
}

#[repr(C)]
#[derive(Default)]
pub struct CColorScheme {
    pub color_scheme_id: ManagedCString,
    pub saber_a_color: Color,
    pub saber_b_color: Color,
    pub environment_color0: Color,
    pub environment_color1: Color,
    pub obstacles_color: Color,
    pub environment_color0_boost: Color,
    pub environment_color1_boost: Color,
}
#[repr(C)]
pub struct CFileSystemBeatmapLevelData {
    pub audio_clip_path: ManagedCString,
    pub audio_data_path: ManagedCString,
    pub name: ManagedCString,
}
#[repr(C)]
pub struct CPreviewMediaData {
    pub level_path: ManagedCString,
    pub cover_sprite: ManagedArray<u8>,
    pub preview_audio_clip: ManagedArray<u8>,
}

/// Loads a `CustomBeatmapLevel` from the given path (zip file or directory).
/// Returns a pointer to a heap-allocated `CustomBeatmapLevel`. Caller must free with `song_core_free_level`.
/// # Parameters
/// - `path`: A pointer to a null-terminated C string representing the path to the song (zip file or directory).
/// - `cache`: A pointer to a `CSongCache` struct representing the song cache (can be null to ignore cache).
/// - `wip`: A boolean indicating whether to load WIP levels.
/// # Safety
/// The `path` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_load_level_path(
    path: *const c_char,
    cache: *mut CSongCache,
    wip: bool,
) -> *mut CCustomBeatmapLevel {
    if path.is_null() {
        panic!("Path is null");
    }

    let path = unsafe {
        CStr::from_ptr(path)
            .to_str()
            .expect("Failed to convert path to str")
    };

    let pathbuf = PathBuf::from(path);
    let beatmap = BeatmapSource::from_path(pathbuf.clone())
        .expect("Failed to create BeatmapSource from path");

    // Prepare cache reference: if `cache` is null, use a temporary in-memory cache.
    let cache_ref = unsafe { cache.as_ref().map(|c| c.inner.as_ref()) };

    let loaded = beatmap
        .load_level(wip, cache_ref)
        .expect("Failed to load beatmap level");

    let Some(loaded) = loaded else {
        return std::ptr::null_mut();
    };

    Box::into_raw(Box::new(loaded.into()))
}

/// Attempts to load a `CustomBeatmapLevel` given a list of directories and a target `leaf` path/name.
/// For each directory, it will try `dir.join(leaf)` and attempt to load. If `leaf` is an absolute path and exists,
/// it will be tried first. Returns pointer to heap-allocated `CustomBeatmapLevel` or panics on failure.
/// # Parameters
/// - `dirs`: A pointer to an array of null-terminated C strings representing directories to search.
/// - `dir_count`: The number of directories in the `dirs` array.
/// - `leaf`: A pointer to a null-terminated C string representing the leaf path/name of the level to load.
/// - `cache`: A pointer to a `CSongCache` struct representing the song cache (can be null to ignore cache).
/// - `wip`: A boolean indicating whether to load WIP levels.
/// # Safety
/// The `dirs` and `leaf` pointers must be valid null-terminated C strings.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_load_level_from_directories(
    dirs: *const *const c_char,
    dir_count: usize,
    cache: *mut CSongCache,
    wip: bool,
) -> *mut ManagedArray<CCustomBeatmapLevel> {
    unsafe {
        if dirs.is_null() {
            panic!("Dirs pointer is null");
        }

        let dir_ptrs = std::slice::from_raw_parts(dirs, dir_count);
        let dir_strings = dir_ptrs
            .iter()
            .map(|&dptr| {
                if dptr.is_null() {
                    panic!("Directory pointer is null");
                }
                CStr::from_ptr(dptr)
                    .to_str()
                    .expect("Failed to convert dir to str")
            })
            .collect::<Vec<&str>>();

        let dirs = dir_strings.iter().map(Path::new).collect::<Vec<&Path>>();

        // Prepare cache reference: if `cache` is null, use a temporary in-memory cache.

        let cache_ref = cache.as_ref().map(|c| c.inner.as_ref());

        let results = level_loader::load_level_from_directories(&dirs, wip, cache_ref);
        let results = match results {
            Ok(levels) => levels,
            Err(e) => panic!("Failed to load levels from directories: {}", e),
        };

        let c_levels: Vec<CCustomBeatmapLevel> =
            results.into_iter().map(CCustomBeatmapLevel::from).collect();

        let boxed = Box::new(ManagedArray::from_vec(c_levels));

        Box::into_raw(boxed)
    }
}

/// Parallel version of `song_core_load_level_from_directories`.
/// Loads custom beatmap levels from the given directories in parallel.
/// Returns a pointer to a heap-allocated `ManagedArray<CCustomBeatmapLevel>`.
/// Caller must free with `song_core_free_level_array`.
/// # Parameters:
/// - `dirs`: A pointer to an array of null-terminated C strings representing directories to search.
/// - `dir_count`: The number of directories in the `dirs` array.
/// - `cache`: A pointer to a `CSongCache` struct representing the song cache (can be null to ignore cache).
/// - `wip`: A boolean indicating whether to load WIP levels.
/// - `callback`: An optional callback function that is called with each loaded CCustomBeatmapLevel,
///   along with its index and the total count.
/// # Safety
/// The `dirs` pointer must be a valid null-terminated C string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_load_levels_from_directories_parallel(
    dirs: *const *const c_char,
    dir_count: usize,
    cache: *mut CSongCache,
    wip: bool,
    user_data: OpaqueUserData,
    callback: Option<extern "C" fn(&CCustomBeatmapLevel, usize, usize, OpaqueUserData)>,
) -> *mut ManagedArray<CCustomBeatmapLevel> {
    unsafe {
        if dirs.is_null() {
            panic!("Dirs pointer is null");
        }

        let dir_ptrs = std::slice::from_raw_parts(dirs, dir_count);
        let dir_strings = dir_ptrs
            .iter()
            .map(|&dptr| {
                if dptr.is_null() {
                    panic!("Directory pointer is null");
                }
                CStr::from_ptr(dptr)
                    .to_str()
                    .expect("Failed to convert dir to str")
            })
            .collect::<Vec<&str>>();

        let dirs = dir_strings.iter().map(Path::new).collect::<Vec<&Path>>();

        // Prepare cache reference: if `cache` is null, use a temporary in-memory cache.

        let cache_ref = cache.as_ref().map(|c| c.inner.as_ref());

        let cb = callback.map(|cb| {
            move |level: &CustomBeatmapLevel, index: usize, total: usize| {
                let c_level = CCustomBeatmapLevel::from(level.clone());
                cb(&c_level, index, total, user_data);
            }
        });

        let results =
            level_loader::load_level_from_directories_parallel(&dirs, wip, cache_ref, cb.as_ref());
        let results = match results {
            Ok(levels) => levels,
            Err(e) => panic!("Failed to load levels from directories: {}", e),
        };

        let c_levels: Vec<CCustomBeatmapLevel> =
            results.into_iter().map(CCustomBeatmapLevel::from).collect();

        let boxed = Box::new(ManagedArray::from_vec(c_levels));

        Box::into_raw(boxed)
    }
}

/// Frees a `CustomBeatmapLevel` allocated by `song_core_load_level_path`.
/// # Parameters
/// - `level`: A pointer to the `CustomBeatmapLevel` to free.
/// # Safety
/// The `level` pointer must be a valid pointer returned by `song_core_load_level_path
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_free_level(level: *mut CCustomBeatmapLevel) {
    if level.is_null() {
        return;
    }
    unsafe {
        let _ = Box::from_raw(level);
    }
}

/// Frees an array of CCustomBeatmapLevel allocated by `song_core_load_level_from_directories`.
/// # Parameters
/// - `levels`: The ManagedArray of CCustomBeatmapLevel to free.
/// # Safety
/// The `levels` pointer must be a valid pointer returned by `song_core_load_level_from_directories`.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn song_core_free_level_array(
    levels: *mut ManagedArray<CCustomBeatmapLevel>,
) {
    unsafe {
        let _ = Box::from_raw(levels);
    };
}

impl From<PreviewMediaData> for CPreviewMediaData {
    fn from(data: PreviewMediaData) -> Self {
        CPreviewMediaData {
            level_path: ManagedCString::from_pathbuf(data.level_path),
            cover_sprite: ManagedArray::from_option(data.cover_sprite),
            preview_audio_clip: ManagedArray::from_option(data.preview_audio_clip),
        }
    }
}

impl From<BeatmapBasicData> for CBeatmapBasicData {
    fn from(data: BeatmapBasicData) -> Self {
        CBeatmapBasicData {
            note_jump_movement_speed: data.note_jump_movement_speed,
            note_jump_start_beat_offset: data.note_jump_start_beat_offset,
            environment: ManagedCString::from_option(data.environment.map(|s| s.0)),
            color_scheme: data
                .color_scheme
                .map(CColorScheme::from)
                .unwrap_or_default(),
            notes_count: data.notes_count.unwrap_or_default(),
            cuttable_objects_count: data.cuttable_objects_count.unwrap_or_default(),
            obstacles_count: data.obstacles_count.unwrap_or_default(),
            bombs_count: data.bombs_count.unwrap_or_default(),
        }
    }
}

impl From<ColorScheme> for CColorScheme {
    fn from(scheme: ColorScheme) -> Self {
        CColorScheme {
            color_scheme_id: ManagedCString::from(scheme.color_scheme_id),
            saber_a_color: scheme.saber_a_color,
            saber_b_color: scheme.saber_b_color,
            environment_color0: scheme.environment_color0,
            environment_color1: scheme.environment_color1,
            obstacles_color: scheme.obstacles_color,
            environment_color0_boost: scheme.environment_color0_boost,
            environment_color1_boost: scheme.environment_color1_boost,
        }
    }
}

impl From<FileSystemBeatmapLevelData> for CFileSystemBeatmapLevelData {
    fn from(data: FileSystemBeatmapLevelData) -> Self {
        CFileSystemBeatmapLevelData {
            audio_clip_path: ManagedCString::from_pathbuf(data.audio_clip_path),
            audio_data_path: data
                .audio_data_path
                .map(ManagedCString::from_pathbuf)
                .unwrap_or_default(),
            name: ManagedCString::from(data.name),
        }
    }
}

impl From<CustomBeatmapLevel> for CCustomBeatmapLevel {
    fn from(level: CustomBeatmapLevel) -> Self {
        let beatmap_basic_datas: Vec<CBeatmapBasicDataTuple> = level
            .beatmap_basic_datas
            .into_iter()
            .map(
                |((characteristic, difficulty), basic_data)| CBeatmapBasicDataTuple {
                    characteristic: ManagedCString::from(characteristic.0),
                    difficulty: ManagedCString::from(difficulty.0),
                    basic_data: CBeatmapBasicData::from(basic_data),
                },
            )
            .collect();

        CCustomBeatmapLevel {
            version: level.version,
            has_precalculated_data: level.has_precalculated_data,
            level_id: ManagedCString::from(level.level_id),
            song_name: ManagedCString::from(level.song_name),
            song_sub_name: ManagedCString::from(level.song_sub_name),
            song_author_name: ManagedCString::from(level.song_author_name),
            all_mappers: ManagedArray::from_vec(
                level
                    .all_mappers
                    .into_iter()
                    .map(ManagedCString::from)
                    .collect(),
            ),
            all_lighters: ManagedArray::from_vec(
                level
                    .all_lighters
                    .into_iter()
                    .map(ManagedCString::from)
                    .collect(),
            ),
            beats_per_minute: level.beats_per_minute,
            integrated_lufs: level.integrated_lufs,
            song_time_offset: level.song_time_offset,
            preview_start_time: level.preview_start_time,
            preview_duration: level.preview_duration,
            song_duration: level.song_duration,
            content_rating: level.content_rating,
            preview_media_data: CPreviewMediaData::from(level.preview_media_data),
            beatmap_basic_datas: ManagedArray::from_vec(beatmap_basic_datas),
            beatmap_level_data: CFileSystemBeatmapLevelData::from(level.beatmap_level_data),
            custom_level_path: ManagedCString::from(
                level.custom_level_path.to_string_lossy().to_string(),
            ),
        }
    }
}
