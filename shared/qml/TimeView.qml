// shared/qml/TimeView.qml
// Vue de configuration du module de temps (similaire à Snaz) avec formatage à la seconde et prévisualisation en direct.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import io.osmv 1.0

Item {
    id: root

    // Helper invisible pour copier le chemin dans le presse-papiers
    TextInput {
        id: clipboardHelper
        visible: false
    }

    // ── Fond de la carte avec lueur Blurple ───────────────────────────────────
    Rectangle {
        id: card
        anchors.fill: parent
        anchors.margins: 14
        radius: theme.radius
        color: theme.bgCard
        border.color: theme.borderAccent
        border.width: 1

        // Halo externe
        Rectangle {
            anchors.fill: parent
            anchors.margins: -6
            radius: parent.radius + 6
            color: "transparent"
            border.color: theme.accentGlow
            border.width: 6
            z: -1
        }

        // ── Contenu défilable ─────────────────────────────────────────────
        ScrollView {
            id: scroll
            anchors.fill: parent
            anchors.margins: 10
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: scroll.width - 20
                spacing: 0

                Item { height: 10 }

                // ── Toggle Enable ─────────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6

                    Switch {
                        id: enableSwitch
                        checked: osmvEngine.time_enabled
                        onCheckedChanged: {
                            osmvEngine.time_enabled = checked
                            osmvEngine.save_time_settings()
                        }

                        indicator: Rectangle {
                            implicitWidth: 44
                            implicitHeight: 24
                            radius: 12
                            color: enableSwitch.checked ? theme.accent : theme.bgInput
                            border.color: enableSwitch.checked ? theme.accent : theme.borderWhite
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 200 } }

                            Rectangle {
                                x: enableSwitch.checked ? parent.width - width - 3 : 3
                                anchors.verticalCenter: parent.verticalCenter
                                width: 18; height: 18
                                radius: 9
                                color: enableSwitch.checked ? "white" : theme.textSub
                                Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                                Behavior on color { ColorAnimation { duration: 200 } }
                            }
                        }
                        contentItem: Text {
                            leftPadding: enableSwitch.indicator.width + 8
                            text: enableSwitch.checked
                                    ? (locale.currentLanguage === "fr" ? "Activer la fonction du temps" : "Enable time function")
                                    : (locale.currentLanguage === "fr" ? "Désactiver la fonction du temps" : "Disable time function")
                            color: enableSwitch.checked ? theme.accent : theme.textSub
                            font.pixelSize: 13
                            font.weight: Font.Bold
                            font.family: "Inter"
                            verticalAlignment: Text.AlignVCenter
                            Behavior on color { ColorAnimation { duration: 200 } }
                        }
                    }
                }

                // ── Séparateur ────────────────────────────────────────────
                Rectangle { Layout.fillWidth: true; height: 1; color: theme.borderWhite; Layout.topMargin: 10; Layout.bottomMargin: 10 }

                // ── Format de sortie ──────────────────────────────────────
                DiscordSection {
                    title: locale.currentLanguage === "fr" ? "Format de sortie" : "Output format"
                    hint: locale.currentLanguage === "fr"
                            ? "Codes disponibles: $h (heures), $m (minutes), $s (secondes), $tt (AM/PM)"
                            : "Available format codes: $h (hours), $m (minutes), $s (seconds), $tt (AM/PM)"
                }

                OsmvTextField {
                    id: formatInput
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    placeholder: "$h:$m:$s $tt"
                    text: osmvEngine.time_format
                    onTextChanged: osmvEngine.time_format = text
                }

                Item { height: 10 }

                // ── Checkbox AM/PM ────────────────────────────────────────
                RowLayout {
                    Layout.leftMargin: 6
                    CheckBox {
                        id: ampmCb
                        checked: osmvEngine.time_ampm
                        onCheckedChanged: osmvEngine.time_ampm = checked
                        contentItem: Text {
                            leftPadding: ampmCb.indicator.width + 4
                            text: locale.currentLanguage === "fr" ? "Format 12 heures Am/Pm ($tt)" : "Times in Am/Pm ($tt)"
                            color: theme.textSub
                            font.pixelSize: 12
                            font.family: "Inter"
                            verticalAlignment: Text.AlignVCenter
                        }
                        indicator: Rectangle {
                            implicitWidth: 18; implicitHeight: 18
                            radius: 4
                            color: ampmCb.checked ? theme.accent : theme.bgInput
                            border.color: ampmCb.checked ? theme.accent : theme.borderWhite
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 150 } }
                            Text {
                                anchors.centerIn: parent
                                visible: ampmCb.checked
                                text: "✓"
                                color: "white"
                                font.pixelSize: 11
                                font.weight: Font.Bold
                            }
                        }
                    }
                }

                // ── Séparateur ────────────────────────────────────────────
                Rectangle { Layout.fillWidth: true; height: 1; color: theme.borderWhite; Layout.topMargin: 10; Layout.bottomMargin: 10 }

                // ── Previsualisations ─────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text {
                            text: locale.currentLanguage === "fr" ? "Heure actuelle du PC" : "Current pc time"
                            color: theme.textSub
                            font.pixelSize: 11
                            font.family: "Inter"
                        }
                        OsmvTextField {
                            Layout.fillWidth: true
                            readOnly: true
                            text: osmvEngine.time_pc_preview
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text {
                            text: locale.currentLanguage === "fr" ? "Aperçu du fichier" : "Live output"
                            color: theme.textSub
                            font.pixelSize: 11
                            font.family: "Inter"
                        }
                        OsmvTextField {
                            Layout.fillWidth: true
                            readOnly: true
                            text: osmvEngine.time_live_preview
                        }
                    }
                }

                // ── Séparateur ────────────────────────────────────────────
                Rectangle { Layout.fillWidth: true; height: 1; color: theme.borderWhite; Layout.topMargin: 10; Layout.bottomMargin: 10 }

                // ── Chemin du fichier ─────────────────────────────────────
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    spacing: 4

                    Text {
                        text: locale.currentLanguage === "fr" ? "Chemin du fichier texte" : "Text file path"
                        color: theme.textSub
                        font.pixelSize: 11
                        font.family: "Inter"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        OsmvTextField {
                            id: pathField
                            Layout.fillWidth: true
                            readOnly: true
                            text: osmvEngine.time_file_path
                        }

                        Rectangle {
                            id: copyBtn
                            width: 100; height: 32
                            radius: theme.radiusSmall
                            color: copyMouse.containsMouse ? theme.accentDim : theme.accent
                            Behavior on color { ColorAnimation { duration: 150 } }
                            scale: copyMouse.pressed ? 0.96 : 1.0
                            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                            Text {
                                anchors.centerIn: parent
                                text: locale.currentLanguage === "fr" ? "Copier" : "Copy Path"
                                color: "white"
                                font.pixelSize: 11
                                font.weight: Font.Bold
                                font.family: "Inter"
                            }

                            MouseArea {
                                id: copyMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    clipboardHelper.text = osmvEngine.time_file_path
                                    clipboardHelper.selectAll()
                                    clipboardHelper.copy()
                                }
                            }
                        }
                    }
                }

                Item { height: 14 }

                // ── Bouton Sauvegarder ────────────────────────────────────
                RowLayout {
                    Layout.leftMargin: 6
                    spacing: 12

                    Rectangle {
                        id: saveBtn
                        width: 150; height: 34
                        radius: theme.radiusSmall
                        color: saveMouseArea.containsMouse ? theme.accentDim : theme.accent
                        Behavior on color { ColorAnimation { duration: 150 } }
                        scale: saveMouseArea.pressed ? 0.96 : 1.0
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                        Text {
                            anchors.centerIn: parent
                            text: locale.t("btn_save")
                            color: "white"
                            font.pixelSize: 13
                            font.weight: Font.Bold
                            font.family: "Inter"
                        }

                        MouseArea {
                            id: saveMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                osmvEngine.save_time_settings()
                                savedFeedback.visible = true
                                savedTimer.restart()
                            }
                        }
                    }

                    Text {
                        id: savedFeedback
                        text: locale.t("feedback_saved")
                        color: theme.accent
                        font.pixelSize: 12
                        font.family: "Inter"
                        visible: false
                        opacity: visible ? 1.0 : 0.0
                        Behavior on opacity { NumberAnimation { duration: 300 } }
                    }
                    Timer {
                        id: savedTimer
                        interval: 2500
                        onTriggered: savedFeedback.visible = false
                    }
                }

                Item { height: 16 }
            }
        }
    }
}
