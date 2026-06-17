// src-rust/main.rs
#![cfg_attr(target_os = "windows", windows_subsystem = "windows")]

mod app;
mod discord;
mod qml_interface;
mod media;
mod utils;

use cxx_qt_lib::{QGuiApplication, QQmlApplicationEngine, QUrl};
use single_instance::SingleInstance;

fn main() {
    // ── Single-instance guard ────────────────────────────────────────────────
    let instance = SingleInstance::new("osmv-obs-stream-music-viewer").unwrap();
    if !instance.is_single() {
        eprintln!("OSMV is already running.");
        return;
    }

    // ── Initialisation de l'application Qt ──────────────────────────────────
    let mut app = QGuiApplication::new();

    // ── Chargement du QML ─────────────────────────────────────────────────────
    let mut engine = QQmlApplicationEngine::new();
    engine
        .pin_mut()
        .load(&QUrl::from("qrc:/qt/qml/io/osmv/shared/qml/main.qml"));

    // ── Boucle d'événements Qt ────────────────────────────────────────────────
    let exit_code = app.pin_mut().exec();

    // Nettoyage à la fermeture : remettre le JSON OBS à null
    utils::write_null_json();

    std::process::exit(exit_code);
}
