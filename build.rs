// build.rs — Compile les bindings CXX-Qt et embarque les ressources Qt QML
use cxx_qt_build::{CxxQtBuilder, QmlModule};

fn main() {
    CxxQtBuilder::new()
        // Module QML exposé via le système de ressources Qt
        // (qml_interface.rs est déclaré dans rust_files, pas besoin de .file() séparé)
        .qml_module(QmlModule {
            uri: "io.osmv",
            rust_files: &["src-rust/qml_interface.rs"],
            qml_files: &[
                "shared/qml/main.qml",
                "shared/qml/OsmvTabButton.qml",
                "shared/qml/NowPlayingView.qml",
                "shared/qml/TimeView.qml",
                "shared/qml/DiscordView.qml",
                "shared/qml/DiscordSection.qml",
                "shared/qml/OsmvSwitch.qml",
                "shared/qml/OsmvTextField.qml",
                "shared/qml/OsmvSeparator.qml",
                "shared/qml/Theme.qml",
                "shared/qml/OsmvLocale.qml",
                "shared/qml/OsmvSettingsView.qml",
                "shared/qml/OsmvHelpView.qml",
            ],
            ..Default::default()
        })
        .build();

    // Windows : intégration de l'icône et des métadonnées dans l'exécutable
    #[cfg(target_os = "windows")]
    {
        let mut res = winresource::WindowsResource::new();
        let exe_dir = std::env::current_dir().unwrap();
        let ico_path = exe_dir.join("windows").join("OSMV.ico");
        if ico_path.exists() {
            res.set_icon(ico_path.to_str().unwrap());
        }
        res.compile().ok();
    }
}
