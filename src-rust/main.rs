// src-rust/main.rs
#![cfg_attr(target_os = "windows", windows_subsystem = "windows")]

mod app;
mod discord;
mod media;
mod qml_interface;
mod utils;

use cxx_qt_lib::{QGuiApplication, QQmlApplicationEngine, QUrl};
use single_instance::SingleInstance;

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let is_gui = args.iter().any(|a| a == "--gui");

    if is_gui {
        run_gui();
    } else {
        run_daemon();
    }
}

fn run_gui() {
    // ── Initialisation de l'application Qt ──────────────────────────────────
    let mut app = QGuiApplication::new();

    // ── Chargement du QML ─────────────────────────────────────────────────────
    let mut engine = QQmlApplicationEngine::new();
    engine
        .pin_mut()
        .load(&QUrl::from("qrc:/qt/qml/io/osmv/shared/qml/main.qml"));

    // ── Boucle d'événements Qt ────────────────────────────────────────────────
    let exit_code = app.pin_mut().exec();

    std::process::exit(exit_code);
}

fn run_daemon() {
    // ── Single-instance guard ────────────────────────────────────────────────
    let instance = SingleInstance::new("osmv-obs-stream-music-viewer").unwrap();
    if !instance.is_single() {
        // Déjà lancé en arrière-plan : on demande simplement l'ouverture de l'interface
        let _ = std::process::Command::new(std::env::current_exe().unwrap())
            .arg("--gui")
            .spawn();
        return;
    }

    // Lancer les processus d'arrière-plan (is_daemon = true)
    let settings = utils::load_settings();
    let _app_state = app::start_background(settings, true);

    // Lancer automatiquement l'interface au premier démarrage
    let _ = std::process::Command::new(std::env::current_exe().unwrap())
        .arg("--gui")
        .spawn();

    // Initialiser GTK pour le Tray Icon sur Linux
    #[cfg(target_os = "linux")]
    if let Err(e) = gtk::init() {
        eprintln!("Failed to initialize GTK for tray icon: {}", e);
    }

    // Charger l'icône
    // On doit créer l'icône, utilisons une fonction helper
    let icon = load_icon();

    use tray_icon::{
        menu::{Menu, MenuEvent, MenuItem},
        TrayIconBuilder, TrayIconEvent,
    };

    let menu = Menu::new();
    let show_item = MenuItem::new("Afficher Configuration", true, None);
    let quit_item = MenuItem::new("Quitter OSMV", true, None);
    let _ = menu.append_items(&[&show_item, &quit_item]);

    let _tray_icon = TrayIconBuilder::new()
        .with_menu(Box::new(menu))
        .with_tooltip("OSMV Background Service")
        .with_icon(icon)
        .build()
        .unwrap();

    let menu_channel = MenuEvent::receiver();
    let tray_channel = TrayIconEvent::receiver();

    // Boucle d'événements
    #[cfg(target_os = "linux")]
    {
        // On Linux, on doit faire tourner gtk::main() sur le thread principal.
        // On vérifie les événements dans un thread séparé.
        std::thread::spawn({
            let show_item_id = show_item.id().clone();
            let quit_item_id = quit_item.id().clone();
            let menu_channel = menu_channel.clone();
            let tray_channel = tray_channel.clone();
            move || loop {
                if check_tray_events(&menu_channel, &tray_channel, &show_item_id, &quit_item_id) {
                    std::process::exit(0);
                }
                std::thread::sleep(std::time::Duration::from_millis(100));
            }
        });

        gtk::main();
    }

    #[cfg(target_os = "windows")]
    {
        let show_item_id = show_item.id().clone();
        let quit_item_id = quit_item.id().clone();
        // Simple sleep loop for Windows if no winit event loop is provided
        // tray-icon on Windows pumps messages in its own thread?
        // Wait, on Windows we usually need a message pump.
        loop {
            if check_tray_events(&menu_channel, &tray_channel, &show_item_id, &quit_item_id) {
                break;
            }
            std::thread::sleep(std::time::Duration::from_millis(50));
        }
    }

    // Quitter
    utils::write_null_json();
    std::process::exit(0);
}

fn check_tray_events(
    menu_channel: &crossbeam_channel::Receiver<tray_icon::menu::MenuEvent>,
    tray_channel: &crossbeam_channel::Receiver<tray_icon::TrayIconEvent>,
    show_item_id: &tray_icon::menu::MenuId,
    quit_item_id: &tray_icon::menu::MenuId,
) -> bool {
    if let Ok(event) = tray_channel.try_recv() {
        // Ignorer les clics de tray pour l'instant afin d'éviter les erreurs d'API changeante
        // L'utilisateur peut utiliser le menu clic droit "Afficher Configuration"
        let _ = event;
    }

    if let Ok(event) = menu_channel.try_recv() {
        if event.id == *show_item_id {
            let _ = std::process::Command::new(std::env::current_exe().unwrap())
                .arg("--gui")
                .spawn();
        } else if event.id == *quit_item_id {
            return true; // Demande de quitter
        }
    }

    // Check if quit.lock exists (created by QML "Quitter" button)
    let quit_path = std::env::temp_dir().join("osmv_quit.lock");
    if quit_path.exists() {
        let _ = std::fs::remove_file(quit_path);
        return true;
    }

    false
}

fn load_icon() -> tray_icon::Icon {
    // Charger l'icône réelle
    let icon_data = include_bytes!("../assets/OSMV_logo.ico");
    let image = image::load_from_memory(icon_data)
        .expect("Failed to load icon")
        .into_rgba8();
    let (width, height) = image.dimensions();
    let rgba = image.into_raw();
    tray_icon::Icon::from_rgba(rgba, width, height).unwrap()
}
