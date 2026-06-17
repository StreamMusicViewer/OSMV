// shared/qml/OsmvSettingsView.qml
// Panneau des paramètres généraux (Langue, Thème) pour l'interface OSMV.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import io.osmv 1.0

Item {
    id: root

    // ── Fond de la carte ─────────────────────────────────────────────────────
    Rectangle {
        id: card
        anchors.fill: parent
        anchors.margins: 14
        radius: theme.radius
        color: theme.bgCard
        border.color: theme.borderAccent
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text {
                text: locale.t("set_title")
                color: theme.textMain
                font.pixelSize: 15
                font.weight: Font.Bold
                font.family: "Inter"
            }

            // Séparateur
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: theme.borderAccent
            }

            // ── Choix de la Langue ──────────────────────────────────────────
            ColumnLayout {
                spacing: 6
                Layout.fillWidth: true

                Text {
                    text: locale.t("set_lang_label")
                    color: theme.textSub
                    font.pixelSize: 12
                    font.family: "Inter"
                }

                RowLayout {
                    spacing: 10

                    // Bouton Français
                    Rectangle {
                        width: 100; height: 32
                        radius: theme.radiusSmall
                        color: locale.currentLanguage === "fr" ? theme.accent : (frMouse.containsMouse ? theme.bgInput : "transparent")
                        border.color: locale.currentLanguage === "fr" ? theme.accent : theme.borderWhite
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "Français"
                            color: locale.currentLanguage === "fr" ? "white" : theme.textMain
                            font.pixelSize: 12
                            font.weight: locale.currentLanguage === "fr" ? Font.Bold : Font.Normal
                            font.family: "Inter"
                        }

                        MouseArea {
                            id: frMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: locale.currentLanguage = "fr"
                        }
                    }

                    // Bouton English
                    Rectangle {
                        width: 100; height: 32
                        radius: theme.radiusSmall
                        color: locale.currentLanguage === "en" ? theme.accent : (enMouse.containsMouse ? theme.bgInput : "transparent")
                        border.color: locale.currentLanguage === "en" ? theme.accent : theme.borderWhite
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "English"
                            color: locale.currentLanguage === "en" ? "white" : theme.textMain
                            font.pixelSize: 12
                            font.weight: locale.currentLanguage === "en" ? Font.Bold : Font.Normal
                            font.family: "Inter"
                        }

                        MouseArea {
                            id: enMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: locale.currentLanguage = "en"
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
