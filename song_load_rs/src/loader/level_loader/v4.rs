use std::{collections::HashMap, path::Path, sync::RwLock};

use tracing::warn;

use crate::{
    beatmap::BeatmapSource,
    cache::SongCache,
    info_dat::InfoDat,
    loader::{
        beatmap_metadata_loader::{self, CUSTOM_LEVEL_PREFIX_ID},
        level_loader::{CustomBeatmapLevel, CustomLevelLoaderError, get_preview_media_data},
    },
    models::{
        beatmap::{
            BeatmapBasicData, BeatmapCharacteristic, BeatmapDifficulty, EnvironmentName,
            FileDifficultyBeatmap, FileSystemBeatmapLevelData, PlayerSensitivityFlag,
        },
        info_dat::{ColorScheme, v4::BeatmapLevelSaveDataV4},
    },
};

pub fn basic_verify_map_v4(beatmap: &BeatmapSource, save_data: &BeatmapLevelSaveDataV4) -> bool {
    // audio must be present
    let audio = &save_data.audio;
    let song_file = &audio.song_filename;
    let cover_file = &save_data.cover_image_filename;
    let audio_file = &audio.audio_data_filename;

    if !beatmap.has_file(song_file) {
        return false;
    }
    if let Some(cover_file) = cover_file
        && !beatmap.has_file(cover_file)
    {
        return false;
    }
    if let Some(audio_file) = audio_file
        && !beatmap.has_file(audio_file)
    {
        return false;
    }

    for diff in save_data.difficulty_beatmaps.iter() {
        let diff_file = &diff.beatmap_data_filename;
        let light_file = &diff.lightshow_data_filename;
        if !beatmap.has_file(diff_file) {
            return false;
        }
        if !beatmap.has_file(light_file) {
            return false;
        }
    }

    true
}

/// Rust translation of `LevelLoader::LoadCustomBeatmapLevel` (V4)
/// Returns `None` on error or missing data.
pub fn load_custom_beatmap_level_v4<C>(
    beatmap: &BeatmapSource,
    wip: bool,
    save: BeatmapLevelSaveDataV4,
    song_cache: Option<&RwLock<C>>,
) -> Result<Option<CustomBeatmapLevel>, CustomLevelLoaderError>
where
    C: SongCache + ?Sized,
{
    if !basic_verify_map_v4(beatmap, &save) {
        warn!("Map {} was missing files!", beatmap);
        return Err(CustomLevelLoaderError::BeatmapVerificationFailed);
    }

    let song_data = beatmap_metadata_loader::load_beatmap_metadata(beatmap, song_cache)?;

    let level_id = format!(
        "{CUSTOM_LEVEL_PREFIX_ID}{}{}",
        song_data.hash,
        if wip { " WIP" } else { "" }
    );

    // destructure song tuple (name, subname, author)
    let song_name = save.song.title.clone().unwrap_or_default();
    let song_sub_name = save.song.sub_title.clone().unwrap_or_default();
    let song_author_name = save.song.author.clone().unwrap_or_default();

    let bpm = save.audio.bpm;
    let lufs = save.audio.lufs.unwrap_or(-6.0);
    let preview_start_time = save.audio.preview_start_time;
    let preview_duration = save.audio.preview_duration;

    let song_duration = song_data.song_length;

    let preview_media_data = get_preview_media_data(
        beatmap,
        save.cover_image_filename.as_deref(),
        &save.audio.song_filename,
    );
    let (beatmap_level_data, beatmap_basic_data_dict) =
        get_beatmap_level_and_basic_data_v4(beatmap, &level_id, &save);

    if beatmap_basic_data_dict.is_empty() {
        return Ok(None);
    }

    let mut all_mappers: Vec<String> = Vec::new();
    let mut all_lighters: Vec<String> = Vec::new();

    for diff in save.difficulty_beatmaps.iter() {
        let Some(diff_authors) = &diff.beatmap_authors else {
            continue;
        };

        for author in diff_authors.mappers.iter().flatten() {
            all_mappers.push(author.clone());
        }
        for author in diff_authors.lighters.iter().flatten() {
            all_lighters.push(author.clone());
        }
    }

    let result = CustomBeatmapLevel {
        version: CustomBeatmapLevel::K_INVALID_VERSION,
        has_precalculated_data: false,
        level_id,
        song_name,
        song_sub_name,
        song_author_name,
        all_mappers,
        all_lighters,
        beats_per_minute: bpm,
        integrated_lufs: lufs,
        song_time_offset: 0.0_f32,
        preview_start_time,
        preview_duration,
        song_duration: song_duration
            .or(song_data.song_length)
            .unwrap_or_default()
            .as_secs_f32(),
        content_rating: PlayerSensitivityFlag::Safe,
        preview_media_data,
        beatmap_basic_datas: beatmap_basic_data_dict,
        custom_level_data: InfoDat::V4(save),
        beatmap_level_data,
        custom_level_path: beatmap.get_real_path().to_path_buf(),
    };

    Ok(Some(result))
}

/// V4 version of GetBeatmapLevelAndBasicData
pub fn get_beatmap_level_and_basic_data_v4(
    beatmap: &BeatmapSource,
    level_id: &str,
    save_data: &BeatmapLevelSaveDataV4,
) -> (
    FileSystemBeatmapLevelData,
    HashMap<(BeatmapCharacteristic, BeatmapDifficulty), BeatmapBasicData>,
) {
    // System::Collections::Generic::Dictionary_2<CharacteristicDifficultyPair, GlobalNamespace::FileDifficultyBeatmap*>
    let mut file_difficulty_beatmaps: HashMap<
        (BeatmapCharacteristic, BeatmapDifficulty),
        FileDifficultyBeatmap,
    > = HashMap::new();
    // System::Collections::Generic::Dictionary_2<CharacteristicDifficultyPair, GlobalNamespace::BeatmapBasicData*>
    let mut basic_data_dict: HashMap<(BeatmapCharacteristic, BeatmapDifficulty), BeatmapBasicData> =
        HashMap::new();

    // Build environment list from provided names or default to empty list
    let environment_names: Vec<EnvironmentName> = save_data
        .environment_names
        .clone()
        .unwrap_or_default()
        .into_iter()
        .map(EnvironmentName)
        .collect();

    // build simple color scheme placeholders (we keep raw strings for now)
    let color_schemes = save_data.color_schemes.clone().unwrap_or_default();

    for diff in &save_data.difficulty_beatmaps {
        let characteristic = BeatmapCharacteristic(diff.characteristic.clone());

        let difficulty = BeatmapDifficulty(diff.difficulty.clone());

        let beatmap_path = diff.beatmap_data_filename.clone();
        if !beatmap.has_file(&beatmap_path) {
            warn!(
                "Diff file '{}' does not exist, skipping...",
                beatmap_path.display()
            );
            continue;
        }

        let lightshow_path = diff.lightshow_data_filename.clone();
        if !beatmap.has_file(&lightshow_path) {
            warn!(
                "Diff Lighting file '{}' does not exist, skipping...",
                lightshow_path.display()
            );
            continue;
        }

        let key = (characteristic.clone(), difficulty.clone());
        if file_difficulty_beatmaps.contains_key(&key) {
            warn!("Duplicate characteristic/difficulty, skipping...");
            continue;
        }

        file_difficulty_beatmaps.insert(
            key.clone(),
            FileDifficultyBeatmap {
                beatmap_path,
                lightshow_path: Some(lightshow_path),
            },
        );

        // collect mappers/lighters
        let mut mappers: Vec<String> = Vec::new();
        let mut lighters: Vec<String> = Vec::new();
        if let Some(authors) = &diff.beatmap_authors {
            if let Some(ms) = &authors.mappers {
                for m in ms.iter() {
                    mappers.push(m.clone());
                }
            }
            if let Some(ls) = &authors.lighters {
                for l in ls.iter() {
                    lighters.push(l.clone());
                }
            }
        }

        let env_name = environment_names.get(diff.environment_name_idx).cloned();

        let color_scheme = diff
            .beatmap_color_scheme_idx
            .and_then(|idx| color_schemes.get(idx).cloned());

        basic_data_dict.insert(
            key,
            BeatmapBasicData {
                note_jump_movement_speed: diff.note_jump_movement_speed,
                note_jump_start_beat_offset: diff.note_jump_start_beat_offset,
                environment: env_name,
                color_scheme: color_scheme.map(ColorScheme::from),
                notes_count: None,
                cuttable_objects_count: None,
                obstacles_count: None,
                bombs_count: None,
                mappers,
                lighters,
            },
        );
    }

    let beatmap_level_data = FileSystemBeatmapLevelData {
        audio_clip_path: save_data.audio.song_filename.clone(),
        audio_data_path: save_data.audio.audio_data_filename.clone(),
        name: level_id.to_string(),
        difficulty_beatmaps: file_difficulty_beatmaps,
    };
    (beatmap_level_data, basic_data_dict)
}

/// Reads info.dat and deserializes as v4 BeatmapLevelSaveDataV4
pub fn get_save_data_from_v4(path: &Path) -> Option<BeatmapLevelSaveDataV4> {
    if path.as_os_str().is_empty() {
        warn!("Provided path was empty!");
        return None;
    }

    let info_path = if path.join("info.dat").exists() {
        path.join("info.dat")
    } else if path.join("Info.dat").exists() {
        path.join("Info.dat")
    } else {
        warn!(
            "no info.dat found for song @ '{}', returning null!",
            path.display()
        );
        return None;
    };

    match std::fs::read_to_string(&info_path) {
        Ok(text) => match serde_json::from_str::<BeatmapLevelSaveDataV4>(&text) {
            Ok(data) => Some(data),
            Err(e) => {
                warn!("Cannot parse info.dat {}: {}", info_path.display(), e);
                None
            }
        },
        Err(e) => {
            warn!(
                "Cannot load file from path: {}! error: {}",
                path.display(),
                e
            );
            None
        }
    }
}

/// Convert a v4 info JSON string into the internal V4 struct
pub fn load_custom_save_data_v4(json_str: &str) -> Option<BeatmapLevelSaveDataV4> {
    match serde_json::from_str::<BeatmapLevelSaveDataV4>(json_str) {
        Ok(s) => Some(s),
        Err(e) => {
            warn!("Failed to parse V4 save data: {}", e);
            None
        }
    }
}
