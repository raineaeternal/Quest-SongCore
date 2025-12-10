use criterion::{Criterion, criterion_group, criterion_main};
use std::path::PathBuf;

fn bench_load_from_zip(c: &mut Criterion) {
    let manifest_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
    let zip_path = manifest_dir
        .join("tests")
        .join("f4c3 (Despacito - cookie).zip");

    c.bench_function("load_from_zip", |b| {
        b.iter(|| {
            let loaded = song_load_rs::song_load::load_song_from_path(zip_path.clone(), None)
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
            let loaded = song_load_rs::song_load::load_song_from_path(dir_path.clone(), None)
                .expect("load from dir failed");
            std::hint::black_box(loaded);
        })
    });
}

criterion_group!(song_load_benches, bench_load_from_zip, bench_load_from_dir);
criterion_main!(song_load_benches);
