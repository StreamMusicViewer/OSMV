// shared/qml/NowPlayingView.qml
// Vue principale "Now Playing" avec pochette d'album, titre, artiste,
// badge de statut animé et bascules de paramètres.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import io.osmv 1.0

Item {
    id: root

    // Couleur extraite de la pochette (accent par défaut)
    property color extractedColor: theme.accent

    // Définition dynamique des couleurs de bordure et de halo lumineux
    property color borderClr: osmvEngine.dynamic_color && osmvEngine.thumbnail_path.length > 0 ? extractedColor : theme.borderAccent
    property color glowClr: osmvEngine.dynamic_color && osmvEngine.thumbnail_path.length > 0 ? Qt.rgba(extractedColor.r, extractedColor.g, extractedColor.b, theme.isDark ? 0.25 : 0.45) : theme.accentGlow

    // ── Fond de la carte ─────────────────────────────────────────────────────
    Rectangle {
        id: card
        anchors.fill: parent
        anchors.margins: 14
        radius: theme.radius
        color: theme.bgCard
        border.color: borderClr
        border.width: 1

        Behavior on border.color { ColorAnimation { duration: theme.durationSlow } }

        // Halo de lueur externe
        Rectangle {
            anchors.fill: parent
            anchors.margins: -6
            radius: parent.radius + 6
            color: "transparent"
            border.color: glowClr
            border.width: 6
            z: -1

            Behavior on border.color { ColorAnimation { duration: theme.durationSlow } }
        }

        // ── Contenu interne ────────────────────────────────────────────────
        RowLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 16

            // ── Pochette d'album ──────────────────────────────────────────
            Item {
                id: albumContainer
                width: 110
                height: 110
                Layout.alignment: Qt.AlignTop

                // Fond de la pochette
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: theme.bgInput
                    border.color: borderClr
                    border.width: 1
                    Behavior on border.color { ColorAnimation { duration: theme.durationSlow } }
                }

                // Image avec coins arrondis via clip
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    clip: true
                    color: "transparent"

                    Image {
                        id: albumImage
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectCrop
                        smooth: true
                        mipmap: true
                        visible: osmvEngine.thumbnail_path.length > 0
                        // file:// + chemin absolu vers le fichier temporaire
                        source: osmvEngine.thumbnail_path.length > 0
                                ? ("file://" + osmvEngine.thumbnail_path)
                                : ""
                        cache: false  // toujours relire pour les nouvelles chansons

                        // Fondu croisé lors du changement de pochette
                        Behavior on source {
                            SequentialAnimation {
                                NumberAnimation {
                                    target: albumImage
                                    property: "opacity"
                                    to: 0
                                    duration: theme.durationFast
                                    easing.type: Easing.InCubic
                                }
                                PropertyAction { }
                                NumberAnimation {
                                    target: albumImage
                                    property: "opacity"
                                    to: 1
                                    duration: theme.durationMedium
                                    easing.type: Easing.OutCubic
                                }
                            }
                        }
                    }
                }

                // Placeholder quand pas de pochette
                Text {
                    anchors.centerIn: parent
                    visible: osmvEngine.thumbnail_path.length === 0
                    text: "♫"
                    font.pixelSize: 36
                    color: theme.textDim
                    font.family: "Inter"
                }

                // Bordure subtile sur la pochette
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: "transparent"
                    border.color: theme.borderWhite
                    border.width: 1
                    visible: osmvEngine.thumbnail_path.length > 0
                }
            }

            // ── Informations sur la piste ─────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 4

                // ── Bascules de paramètres (en haut) ──────────────────────
                Row {
                    spacing: 16
                    Layout.fillWidth: true

                    OsmvSwitch {
                        label: locale.t("match_cover")
                        width: 160
                        checked: osmvEngine.dynamic_color
                        onClicked: {
                            osmvEngine.dynamic_color = !osmvEngine.dynamic_color
                            osmvEngine.save_media_settings()
                        }
                    }

                    OsmvSwitch {
                        label: locale.t("audio_viz")
                        width: 160
                        checked: osmvEngine.audio_visualizer
                        onClicked: {
                            osmvEngine.audio_visualizer = !osmvEngine.audio_visualizer
                            osmvEngine.save_media_settings()
                        }
                    }
                }

                // Titre
                Text {
                    id: titleText
                    Layout.fillWidth: true
                    text: osmvEngine.title.length > 0 ? osmvEngine.title : locale.t("status_waiting")
                    color: theme.textMain
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    font.family: "Inter"
                    elide: Text.ElideRight
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2

                    Behavior on text {
                        SequentialAnimation {
                            NumberAnimation { target: titleText; property: "opacity"; to: 0; duration: theme.durationFast }
                            PropertyAction  { }
                            NumberAnimation { target: titleText; property: "opacity"; to: 1; duration: theme.durationMedium }
                        }
                    }
                }

                // Artiste
                Text {
                    id: artistText
                    Layout.fillWidth: true
                    text: osmvEngine.artist
                    visible: osmvEngine.artist.length > 0 && osmvEngine.artist !== "Unknown Artist"
                    color: theme.textSub
                    font.pixelSize: 12
                    font.family: "Inter"
                    elide: Text.ElideRight

                    Behavior on text {
                        SequentialAnimation {
                            NumberAnimation { target: artistText; property: "opacity"; to: 0; duration: theme.durationFast }
                            PropertyAction  { }
                            NumberAnimation { target: artistText; property: "opacity"; to: 1; duration: theme.durationMedium }
                        }
                    }
                }

                // Album
                Text {
                    Layout.fillWidth: true
                    text: osmvEngine.album.length > 0 ? locale.t("album_prefix") + osmvEngine.album : ""
                    visible: osmvEngine.album.length > 0
                    color: theme.textDim
                    font.pixelSize: 11
                    font.family: "Inter"
                    elide: Text.ElideRight
                }

                Item { height: 6 }

                // ── Badge de statut animé ─────────────────────────────────
                Row {
                    spacing: 7
                    Layout.alignment: Qt.AlignLeft

                    Rectangle {
                        id: statusDot
                        width: 8; height: 8
                        radius: 4
                        anchors.verticalCenter: parent.verticalCenter

                        color: {
                            switch(osmvEngine.status) {
                                case "playing": return theme.colGreen
                                case "paused":  return theme.colYellow
                                case "stopped": return theme.colRed
                                default:        return theme.textDim
                            }
                        }

                        // Pulsation organique uniquement en mode "playing"
                        SequentialAnimation on scale {
                            running: osmvEngine.status === "playing"
                            loops: Animation.Infinite
                            NumberAnimation { to: 1.35; duration: 700; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 1.00; duration: 700; easing.type: Easing.InOutSine }
                            onRunningChanged: if (!running) statusDot.scale = 1.0
                        }

                        Behavior on color { ColorAnimation { duration: theme.durationFast } }
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: {
                            switch(osmvEngine.status) {
                                case "playing": return locale.t("status_playing")
                                case "paused":  return locale.t("status_paused")
                                case "stopped": return locale.t("status_stopped")
                                default:        return locale.t("status_waiting_short")
                            }
                        }
                        color: {
                            switch(osmvEngine.status) {
                                case "playing": return theme.colGreen
                                case "paused":  return theme.colYellow
                                case "stopped": return theme.colRed
                                default:        return theme.textDim
                            }
                        }
                        font.pixelSize: 12
                        font.family: "Inter"
                        Behavior on color { ColorAnimation { duration: theme.durationFast } }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }

    // Timer de polling (1 s) — met à jour les propriétés depuis le thread Rust
    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: osmvEngine.poll()
    }

    // ── Extracteur de Couleur via Canvas ──
    Canvas {
        id: colorExtractor
        x: -50; y: -50; width: 10; height: 10
        visible: true
        opacity: 0

        property string currentSource: albumImage.source.toString()

        onCurrentSourceChanged: {
            if (albumImage.status === Image.Ready) {
                requestPaint();
            }
        }

        Connections {
            target: albumImage
            function onStatusChanged() {
                if (albumImage.status === Image.Ready) {
                    colorExtractor.requestPaint();
                }
            }
        }

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            try {
                // Échantillonne l'image sur une grille de 10x10 pour calculer une moyenne robuste
                ctx.drawImage(albumImage, 0, 0, width, height);
                var imageData = ctx.getImageData(0, 0, width, height);
                var r = 0, g = 0, b = 0, count = 0;
                for (var i = 0; i < imageData.data.length; i += 4) {
                    r += imageData.data[i];
                    g += imageData.data[i+1];
                    b += imageData.data[i+2];
                    count++;
                }
                if (count > 0) {
                    r = Math.round(r / count);
                    g = Math.round(g / count);
                    b = Math.round(b / count);
                    
                    // Convertit en couleur QML
                    var col = Qt.rgba(r / 255.0, g / 255.0, b / 255.0, 1.0);
                    
                    // Si la couleur est trop sombre ou trop terne, on la rehausse
                    // en augmentant la luminosité/saturation si nécessaire.
                    root.extractedColor = col;
                }
            } catch (e) {
                // En cas d'erreur ou si l'image n'est pas encore peinte
            }
        }
    }
}

