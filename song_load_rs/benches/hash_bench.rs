use criterion::{Criterion, criterion_group, criterion_main};
use song_load_rs::models::InfoDat;
use std::path::PathBuf;

// Benchmarks for hashing functions in `song_load_rs::hash`.
// - `hash_from_zip` benchmarks `compute_custom_level_from_path` on the zip file
// - `hash_from_dir` benchmarks `compute_custom_level_from_path` on the unpacked dir
// - `create_sha1_in_memory` prepares required files in-memory and benchmarks
//   `create_sha1_from_path_bytes` to measure pure hashing cost without disk IO.

fn bench_hash_from_zip(c: &mut Criterion) {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let zip_path = manifest_dir
        .join("tests")
        .join("f4c3 (Despacito - cookie).zip");

    let beatmap =
        song_load_rs::beatmap::Beatmap::from_path(&zip_path).expect("load beatmap from zip");

    c.bench_function("hash_from_zip", |b| {
        b.iter(|| {
            let hash = song_load_rs::hash::compute_custom_level_hash_from_beatmap(&beatmap)
                .expect("compute hash from zip failed");
            std::hint::black_box(hash);
        })
    });
}

fn bench_hash_from_dir(c: &mut Criterion) {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let dir_path = manifest_dir.join("tests").join("f4c3 (Despacito - cookie)");

    let beatmap =
        song_load_rs::beatmap::Beatmap::from_path(&dir_path).expect("load beatmap from dir");

    c.bench_function("hash_from_dir", |b| {
        b.iter(|| {
            let hash = song_load_rs::hash::compute_custom_level_hash_from_beatmap(&beatmap)
                .expect("compute hash from dir failed");
            std::hint::black_box(hash);
        })
    });
}

fn bench_create_sha1_in_memory(c: &mut Criterion) {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let dir_path = manifest_dir.join("tests").join("f4c3 (Despacito - cookie)");

    // Prepare in-memory info bytes and files list once (outside the inner loop).
    let info_path = dir_path.join("Info.dat");
    let info_bytes = std::fs::read(&info_path).expect("read Info.dat");
    let info_contents = String::from_utf8(info_bytes.clone()).expect("info utf8");
    let info_dat: InfoDat = serde_json::from_str(&info_contents).expect("parse info dat");

    let files_vec: Vec<(std::path::PathBuf, bytes::Bytes)> =
        song_load_rs::hash::necessary_files_from_info_dat(&info_dat)
            .into_iter()
            .filter_map(|p| {
                let full = dir_path.join(&p);
                if !(full.exists() && full.is_file()) {
                    return None;
                }
                match std::fs::read(&full) {
                    Ok(b) => Some((p, bytes::Bytes::from(b))),
                    Err(_) => None,
                }
            })
            .collect();

    let prepend = info_bytes; // &[u8]

    c.bench_function("create_sha1_in_memory", |b| {
        b.iter(|| {
            // use iter().cloned() to provide owned (PathBuf, Bytes) pairs each iteration
            let h = song_load_rs::hash::create_sha1_from_path_bytes(
                &prepend,
                files_vec.iter().cloned(),
            )
            .expect("create sha1 failed");
            std::hint::black_box(h);
        })
    });
}

criterion_group!(
    hash_benches,
    bench_hash_from_zip,
    bench_hash_from_dir,
    bench_create_sha1_in_memory
);
criterion_main!(hash_benches);
