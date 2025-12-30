use std::path::Path;

use semver::Version;
use serde_json::Value as JsonValue;
use std::collections::HashMap;
use std::hash::{Hash, Hasher};
use tracing::warn;

use crate::{
    cache::SongCache,
    models::{
        v2::{self, StandardLevelInfoSaveDataV2},
        v4::{self, BeatmapLevelSaveDataV4},
    },
    song_loader::{self, CUSTOM_LEVEL_PREFIX_ID},
};

#[derive(Debug, Clone)]
pub struct IBeatmapLevelData;
#[derive(Debug, Clone)]
pub struct IPreviewMediaData;
#[derive(Debug, Clone)]
pub struct BeatmapCharacteristicSO;
#[derive(Debug, Clone)]
pub struct BeatmapDifficulty;
#[derive(Debug, Clone)]
pub struct BeatmapBasicData;
#[derive(Debug, Clone)]
pub struct BeatmapKey;

#[derive(Debug, Clone)]
enum PlayerSensitivityFlag {
    Unknown,
    Safe,
    Themes,
    Explicit,
}

#[derive(Debug, Clone)]
pub struct CustomBeatmapLevel {
    pub version: i32,
    pub has_precalculated_data: bool,
    pub level_id: String,
    pub song_name: String,
    pub song_sub_name: String,
    pub song_author_name: String,
    pub all_mappers: Vec<String>,
    pub all_lighters: Vec<String>,
    pub beats_per_minute: f32,
    pub integrated_lufs: f32,
    pub song_time_offset: f32,
    pub preview_start_time: f32,
    pub preview_duration: f32,
    pub song_duration: f32,
    pub content_rating: PlayerSensitivityFlag,
    pub preview_media_data: IPreviewMediaData,
    pub beatmap_basic_datas:
        std::collections::HashMap<(BeatmapCharacteristicSO, BeatmapDifficulty), BeatmapBasicData>,
    pub characteristics_cache: Option<Vec<BeatmapCharacteristicSO>>,
    pub beatmap_keys_cache: Option<Vec<BeatmapKey>>,

    custom_level_data: StandardLevelInfoSaveData,
    beatmap_level_data: IBeatmapLevelData,
    custom_level_path: String,
    //     CustomJSONData::CustomLevelInfoSaveDataV2* _customLevelSaveDataV2;
    // CustomJSONData::CustomBeatmapLevelSaveDataV4* _customBeatmapLevelSaveDataV4;
    // GlobalNamespace::IBeatmapLevelData* _beatmapLevelData;
    // std::string _customLevelPath;
}

impl CustomBeatmapLevel {
    const K_INVALID_VERSION: i32 = -1;
}

#[derive(Debug, Clone)]
pub enum StandardLevelInfoSaveData {
    V2(v2::StandardLevelInfoSaveDataV2),
    V4(v4::BeatmapLevelSaveDataV4),
}

pub fn basic_verify_map_v2(level_path: &Path, save_data: &StandardLevelInfoSaveDataV2) -> bool {
    let song_file = &save_data.song_filename;
    let cover_file = &save_data.cover_image_filename;

    if let Some(song_file) = song_file
        && !level_path.join(song_file).exists()
    {
        return false;
    }
    if let Some(cover_file) = cover_file
        && !level_path.join(cover_file).exists()
    {
        return false;
    }

    for set in save_data.difficulty_beatmap_sets.iter().flatten() {
        for diff in set.difficulty_beatmaps.iter().flatten() {
            if diff.beatmap_filename.is_none()
                || !level_path
                    .join(&diff.beatmap_filename.as_ref().unwrap())
                    .exists()
            {
                return false;
            }
        }
    }

    true
}
pub fn basic_verify_map_v4(level_path: &Path, save_data: &BeatmapLevelSaveDataV4) -> bool {
    // audio must be present
    let audio = match &save_data.audio {
        Some(a) => a,
        None => return false,
    };

    let song_file = &audio.song_filename;
    let cover_file = &save_data.cover_image_filename;
    let audio_file = &audio.audio_data_filename;

    if !level_path.join(song_file).exists() {
        return false;
    }
    if let Some(cover_file) = cover_file
        && !level_path.join(cover_file).exists()
    {
        return false;
    }
    if let Some(audio_file) = audio_file
        && !level_path.join(audio_file).exists()
    {
        return false;
    }

    for diff in save_data.difficulty_beatmaps.iter().flatten() {
        let diff_file = &diff.beatmap_data_filename;
        let light_file = &diff.lightshow_data_filename;
        if let Some(diff_file) = diff_file
            && !level_path.join(diff_file).exists()
        {
            return false;
        }
        if let Some(light_file) = light_file
            && !level_path.join(light_file).exists()
        {
            return false;
        }
    }

    true
}

/// Rust translation of `LevelLoader::LoadCustomBeatmapLevel` (V4)
/// Returns `None` on error or missing data.
pub fn load_custom_beatmap_level_v4(
    level_path: &Path,
    wip: bool,
    save_data: Option<&BeatmapLevelSaveDataV4>,
    song_cache: &mut impl SongCache,
) -> Option<(CustomBeatmapLevel, String)> {
    let save = match save_data {
        Some(s) => s,
        None => {
            warn!("saveData was null for level @ {}", level_path.display());
            if cfg!(feature = "throw_on_missing_data") {
                panic!("saveData was null for level @ {}", level_path.display());
            }
            return None;
        }
    };

    if !basic_verify_map_v4(level_path, save) {
        warn!("Map {} was missing files!", level_path.display());
        if cfg!(feature = "throw_on_missing_data") {
            panic!("Map {} was missing files!", level_path.display());
        }
        return None;
    }

    let song_data =
        song_loader::load_song_from_path(level_path.to_path_buf(), Some(song_cache)).ok()?;

    let level_id = format!(
        "{CUSTOM_LEVEL_PREFIX_ID}{}{}",
        song_data.hash,
        if wip { " WIP" } else { "" }
    );

    // destructure song tuple (name, subname, author)
    let mut song_name = save.song.title.unwrap_or_default();
    let mut song_sub_name = save.song.sub_title.unwrap_or_default();
    let mut song_author_name = save.song.author.unwrap_or_default();

    let bpm = save.audio.bpm;
    let lufs = save.audio.lufs.unwrap_or(-6.0);
    let preview_start_time = save.audio.preview_start_time;
    let preview_duration = save.audio.preview_duration;

    let song_duration = song_data.song_length;

    let preview_media_data = crate::media::get_preview_media_data(
        level_path,
        &save.cover_image_filename,
        &save.audio.song_filename,
    );
    let (beatmap_level_data, beatmap_basic_data) =
        crate::loader::get_beatmap_level_and_basic_data(level_path, &level_id, save);

    if beatmap_basic_data.count() == 0 {
        return None;
    }

    let mut all_mappers: Vec<String> = Vec::new();
    let mut all_lighters: Vec<String> = Vec::new();

    for diff in save.difficulty_beatmaps.iter().flatten() {
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

    let result = CustomBeatmapLevel::new(
        level_path.to_string_lossy().to_string(),
        StandardLevelInfoSaveData::V4(save),
        beatmap_level_data,
        false,
        level_id,
        song_name,
        song_sub_name,
        song_author_name,
        all_mappers,
        all_lighters,
        bpm,
        lufs,
        0.0_f32,
        preview_start_time,
        preview_duration,
        song_duration,
        PlayerSensitivityFlag::Safe,
        preview_media_data.into_ipreview_media_data(),
        beatmap_basic_data,
    );

    Some(result)
}

impl PartialEq for BeatmapCharacteristicSO {
    fn eq(&self, other: &Self) -> bool {
        format!("{:?}", self) == format!("{:?}", other)
    }
}
impl Eq for BeatmapCharacteristicSO {}
impl Hash for BeatmapCharacteristicSO {
    fn hash<H: Hasher>(&self, state: &mut H) {
        format!("{:?}", self).hash(state);
    }
}

impl PartialEq for BeatmapDifficulty {
    fn eq(&self, other: &Self) -> bool {
        format!("{:?}", self) == format!("{:?}", other)
    }
}
impl Eq for BeatmapDifficulty {}
impl Hash for BeatmapDifficulty {
    fn hash<H: Hasher>(&self, state: &mut H) {
        format!("{:?}", self).hash(state);
    }
}

impl Default for BeatmapBasicData {
    fn default() -> Self {
        BeatmapBasicData
    }
}

// V2 | V3
/// Rough Rust translation of the original GetBeatmapLevelAndBasicData.
/// This implementation follows the original control flow and error handling (warnings & skips).
/// It returns a placeholder IBeatmapLevelData and a map of basic data keyed by (characteristic, difficulty).
pub fn get_beatmap_level_and_basic_data_v2(
    level_path: &Path,
    level_id: &str,
    save_data: &v2::StandardLevelInfoSaveDataV2,
    environment_names: &[String],
    color_schemes: &[Option<String>],
) -> (
    IBeatmapLevelData,
    HashMap<(BeatmapCharacteristicSO, BeatmapDifficulty), BeatmapBasicData>,
) {
    let mut file_difficulty_beatmaps: HashMap<
        (BeatmapCharacteristicSO, BeatmapDifficulty),
        String,
    > = HashMap::new();
    let mut basic_data_dict: HashMap<
        (BeatmapCharacteristicSO, BeatmapDifficulty),
        BeatmapBasicData,
    > = HashMap::new();

    for beatmap_set in save_data.difficulty_beatmap_sets.iter().flatten() {
        // placeholder: resolve characteristic by serialized name
        let characteristic = match beatmap_characteristics::get_by_serialized_name(
            &beatmap_set.beatmap_characteristic_name,
        ) {
            Some(c) => c,
            None => {
                warn!(
                    "Got null characteristic for characteristic name {}, skipping...",
                    beatmap_set.beatmap_characteristic_name
                );
                continue;
            }
        };

        for difficulty_beatmap in beatmap_set.difficulty_beatmaps.iter().flatten() {
            // parse difficulty string into BeatmapDifficulty (placeholder parse)
            let difficulty = match crate::beatmap_difficulty::from_serialized_name(
                &difficulty_beatmap.difficulty,
            ) {
                Some(d) => d,
                None => {
                    warn!(
                        "Failed to parse a diff string: {}, skipping...",
                        difficulty_beatmap.difficulty
                    );
                    continue;
                }
            };

            let beatmap_filename = match &difficulty_beatmap.beatmap_filename {
                Some(s) => s,
                None => {
                    warn!("Beatmap entry missing filename, skipping...");
                    continue;
                }
            };
            let beatmap_path = level_path.join(beatmap_filename);
            if !beatmap_path.exists() {
                warn!(
                    "Diff file '{}' does not exist, skipping...",
                    beatmap_path.display()
                );
                continue;
            }

            let key = (characteristic.clone(), difficulty.clone());
            if file_difficulty_beatmaps.contains_key(&key) {
                warn!(
                    "Duplicate characteristic/difficulty: {}/{}",
                    beatmap_set.beatmap_characteristic_name,
                    format!("{:?}", difficulty)
                );
                continue;
            }

            file_difficulty_beatmaps
                .insert(key.clone(), beatmap_path.to_string_lossy().to_string());

            // compute envNameIndex
            let save_data_had_env_names =
                !save_data.environment_names.unwrap_or_default().is_empty();

            let mut env_name_index = if save_data_had_env_names {
                difficulty_beatmap.environment_name_idx
            } else if crate::beatmap_characteristics::contains_rotation_events(&characteristic) {
                1
            } else {
                0
            };
            // clamp to valid range
            env_name_index = env_name_index.clamp(0, environment_names.len().saturating_sub(1));

            let color_scheme = difficulty_beatmap
                .beatmap_color_scheme_idx
                .and_then(|idx| color_schemes.get(idx as usize).cloned())
                .flatten();

            // Build BeatmapBasicData -- here we fill with a default placeholder.
            // In a full implementation map the concrete fields (njm, offset, env name, color scheme, etc).
            basic_data_dict.insert(
                key,
                BeatmapBasicData::new(
                    difficulty_beatmap.note_jump_movement_speed,
                    difficulty_beatmap.note_jump_start_beat_offset,
                    environment_names.get(env_name_index_usize).cloned(),
                    color_scheme,
                ),
            );
        }
    }

    // Construct a placeholder IBeatmapLevelData representing the filesystem level data.
    // A full implementation would create a FileSystemBeatmapLevelData-like structure with the collected dict.
    let beatmap_level_data = IBeatmapLevelData;

    (beatmap_level_data, basic_data_dict)
}
