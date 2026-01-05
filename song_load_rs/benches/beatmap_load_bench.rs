use criterion::{Criterion, criterion_group, criterion_main};
use song_load_rs::{
    cache::mem_cache::MemCache,
    beatmap::BeatmapSource,
    loader::beatmap_metadata_loader,
};
use tokio::runtime::Runtime;
use std::path::PathBuf;

fn bench_load_from_zip(c: &mut Criterion) {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let zip_path = manifest_dir
        .join("tests")
        .join("f4c3 (Despacito - cookie).zip");

    c.bench_function("load_from_zip", |b| {
        b.iter(|| {
            let loaded = beatmap_metadata_loader::load_beatmap_from_path::<MemCache>(zip_path.clone(), None, false)
                .expect("load from zip failed");
            // Keep the result in scope so it's not optimized away
            std::hint::black_box(loaded);
        })
    });
}

fn bench_load_from_dir(c: &mut Criterion) {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let dir_path = manifest_dir.join("tests").join("f4c3 (Despacito - cookie)");

    c.bench_function("load_from_dir", |b| {
        b.iter(|| {
            let loaded = beatmap_metadata_loader::load_beatmap_from_path::<MemCache>(dir_path.clone(), None, false)
                .expect("load from dir failed");
            std::hint::black_box(loaded);
        })
    });
}


fn bench_load_from_zip_async(c: &mut Criterion) {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let zip_path = manifest_dir
        .join("tests")
        .join("f4c3 (Despacito - cookie).zip");

    let rt = Runtime::new().expect("failed to create runtime");

    c.bench_function("load_from_zip_async", |b| {
        b.iter(|| {
            rt.block_on(async {
                let beatmap = BeatmapSource::from_path_async(zip_path.clone())
                    .await
                    .expect("from_path_async failed");
                let loaded = beatmap_metadata_loader::load_beatmap_metadata_async::<MemCache>(
                    &beatmap,
                    None,
                    false,
                )
                .await
                .expect("async load from zip failed");
                std::hint::black_box(loaded);
            })
        })
    });
}

fn bench_load_from_dir_async(c: &mut Criterion) {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let dir_path = manifest_dir.join("tests").join("f4c3 (Despacito - cookie)");

    let rt = Runtime::new().expect("failed to create runtime");

    c.bench_function("load_from_dir_async", |b| {
        b.iter(|| {
            rt.block_on(async {
                let beatmap = BeatmapSource::from_path_async(dir_path.clone())
                    .await
                    .expect("from_path_async failed");
                let loaded = beatmap_metadata_loader::load_beatmap_metadata_async::<MemCache>(
                    &beatmap,
                    None,
                    false,
                )
                .await
                .expect("async load from dir failed");
                std::hint::black_box(loaded);
            })
        })
    });
}

criterion_group!(
    song_load_benches_async,
    bench_load_from_zip_async,
    bench_load_from_dir_async
);
criterion_group!(song_load_benches, bench_load_from_zip, bench_load_from_dir);
criterion_main!(song_load_benches, song_load_benches_async);
