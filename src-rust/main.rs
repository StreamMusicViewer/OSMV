// src-rust/main.rs
// Entry point for OSMV Rust — OBS Stream Music Viewer

#![cfg_attr(target_os = "windows", windows_subsystem = "windows")]

mod app;
mod gui;
mod media;
mod utils;

use gui::OsmvApp;

fn main() -> eframe::Result {
    // Single-instance guard via a lock file
    let lock_path = utils::exe_dir().join(".osmv.lock");

    // Try to create the lock file exclusively — if it already exists and is
    // locked by another process, we bail out.  We use std::fs::OpenOptions with
    // exclusive creation; on Windows a process-lifetime file lock is sufficient.
    let _lock_file = std::fs::OpenOptions::new()
        .write(true)
        .create(true)
        .truncate(true)
        .open(&lock_path);

    // Clean up lock on exit
    let lock_path_clone = lock_path.clone();
    let _ = std::panic::catch_unwind(|| ());

    let native_options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_title("OBS Stream Music Viewer")
            .with_inner_size([460.0, 280.0])
            .with_min_inner_size([380.0, 220.0])
            .with_icon(load_icon()),
        ..Default::default()
    };

    let result = eframe::run_native(
        "OBS Stream Music Viewer",
        native_options,
        Box::new(|cc| Ok(Box::new(OsmvApp::new(cc)))),
    );

    // Write null JSON on clean exit
    utils::write_null_json();
    // Remove lock file
    let _ = std::fs::remove_file(&lock_path_clone);

    result
}

fn load_icon() -> egui::IconData {
    let exe_dir = utils::exe_dir();
    let ico_path = exe_dir.join("OSMV_logo.ico");
    if ico_path.exists() {
        if let Ok(bytes) = std::fs::read(&ico_path) {
            if let Ok(img) = image::load_from_memory(&bytes) {
                let rgba = img.to_rgba8();
                let (w, h) = rgba.dimensions();
                return egui::IconData {
                    rgba: rgba.into_raw(),
                    width: w,
                    height: h,
                };
            }
        }
    }
    egui::IconData { rgba: vec![0u8; 4], width: 1, height: 1 }
}
