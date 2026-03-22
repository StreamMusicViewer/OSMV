// src-rust/main.rs
#![cfg_attr(target_os = "windows", windows_subsystem = "windows")]

mod app;
mod discord;
mod gui;
mod media;
mod utils;

use gui::OsmvApp;
use single_instance::SingleInstance;

// Embed the icon at compile time from the assets folder
const ICON_BYTES: &[u8] = include_bytes!("../assets/OSMV_logo.ico");

fn main() -> eframe::Result {
    // ── Single-instance guard ────────────────────────────────────────────────
    let instance = SingleInstance::new("osmv-obs-stream-music-viewer").unwrap();
    if !instance.is_single() {
        // On Windows, show a native dialog; on Linux print to stderr
        #[cfg(target_os = "windows")]
        {
            // Simple MessageBox via windows crate is tricky without the feature;
            // use a small eframe dialog instead
            let opts = eframe::NativeOptions {
                viewport: egui::ViewportBuilder::default()
                    .with_title("OSMV already running")
                    .with_inner_size([340.0, 100.0])
                    .with_resizable(false),
                ..Default::default()
            };
            return eframe::run_native(
                "OSMV already running",
                opts,
                Box::new(|_cc| {
                    Ok(Box::new(AlreadyRunningApp))
                }),
            );
        }
        #[cfg(not(target_os = "windows"))]
        {
            eprintln!("OSMV is already running.");
            return Ok(());
        }
    }

    let native_options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_title("OBS Stream Music Viewer")
            .with_inner_size([500.0, 340.0])
            .with_min_inner_size([420.0, 300.0])
            .with_icon(load_icon()),
        ..Default::default()
    };

    let result = eframe::run_native(
        "OBS Stream Music Viewer",
        native_options,
        Box::new(|cc| Ok(Box::new(OsmvApp::new(cc)))),
    );

    utils::write_null_json();
    result
}

fn load_icon() -> egui::IconData {
    // Extract the first image from the embedded .ico file
    if let Ok(img) = image::load_from_memory(ICON_BYTES) {
        let rgba = img.to_rgba8();
        let (w, h) = rgba.dimensions();
        return egui::IconData {
            rgba: rgba.into_raw(),
            width: w,
            height: h,
        };
    }
    egui::IconData { rgba: vec![0u8; 4], width: 1, height: 1 }
}

// ── Tiny "already running" popup app ────────────────────────────────────────
struct AlreadyRunningApp;
impl eframe::App for AlreadyRunningApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.vertical_centered(|ui| {
                ui.add_space(16.0);
                ui.label(egui::RichText::new("⚠ OBS Stream Music Viewer is already running.")
                    .size(13.0));
                ui.add_space(8.0);
                ui.label("Check the system tray.");
                ui.add_space(12.0);
                if ui.button("OK").clicked() {
                    ctx.send_viewport_cmd(egui::ViewportCommand::Close);
                }
            });
        });
    }
}
