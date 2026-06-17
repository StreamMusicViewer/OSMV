
// shared/qml/DiscordView.qml
// Vue de configuration Discord RPC avec défilement fluide,
// focus animé sur les champs de saisie et bouton de sauvegarde.

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import io.osmv 1.0

Item {
    id: root

    // ── Fond de la carte avec lueur Blurple ───────────────────────────────────
    Rectangle {
        id: card
        anchors.fill: parent
        anchors.margins: 14
        radius: theme.radius
        color: theme.bgCard
        border.color: theme.borderBlurple
        border.width: 1

        // Halo externe blurple
        Rectangle {
            anchors.fill: parent
            anchors.margins: -6
            radius: parent.radius + 6
            color: "transparent"
            border.color: theme.blurpleGlow
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
                        checked: osmvEngine.dc_enabled
                        onCheckedChanged: osmvEngine.dc_enabled = checked

                        indicator: Rectangle {
                            implicitWidth: 44
                            implicitHeight: 24
                            radius: 12
                            color: enableSwitch.checked ? theme.blurple : theme.bgInput
                            border.color: enableSwitch.checked ? theme.blurple : theme.borderWhite
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
                                    ? locale.t("dc_on")
                                    : locale.t("dc_off")
                            color: enableSwitch.checked ? theme.blurple : theme.textSub
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

                // ── Application Client ID ─────────────────────────────────
                DiscordSection { title: locale.t("dc_section_client"); hint: locale.t("dc_hint_client") }

                OsmvTextField {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    placeholder: "1234567890123456789"
                    text: osmvEngine.dc_client_id
                    onTextChanged: osmvEngine.dc_client_id = text
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: theme.borderWhite; Layout.topMargin: 10; Layout.bottomMargin: 10 }

                // ── Intégration Musique ───────────────────────────────────
                DiscordSection { title: locale.t("dc_section_music"); hint: "" }

                RowLayout {
                    Layout.leftMargin: 6
                    CheckBox {
                        id: useMusicCb
                        checked: osmvEngine.dc_use_music
                        onCheckedChanged: osmvEngine.dc_use_music = checked
                        contentItem: Text {
                            leftPadding: useMusicCb.indicator.width + 4
                            text: locale.t("dc_music_cb")
                            color: theme.textSub
                            font.pixelSize: 12
                            font.family: "Inter"
                            verticalAlignment: Text.AlignVCenter
                        }
                        indicator: Rectangle {
                            implicitWidth: 18; implicitHeight: 18
                            radius: 4
                            color: useMusicCb.checked ? theme.blurple : theme.bgInput
                            border.color: useMusicCb.checked ? theme.blurple : theme.borderWhite
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 150 } }
                            Text {
                                anchors.centerIn: parent
                                visible: useMusicCb.checked
                                text: "✓"
                                color: "white"
                                font.pixelSize: 11
                                font.weight: Font.Bold
                            }
                        }
                    }
                }

                Text {
                    Layout.leftMargin: 6
                    text: locale.t("dc_music_hint")
                    color: theme.textDim
                    font.pixelSize: 11
                    font.family: "Inter"
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: theme.borderWhite; Layout.topMargin: 10; Layout.bottomMargin: 10 }

                // ── Activité par défaut ───────────────────────────────────
                DiscordSection { title: locale.t("dc_section_default"); hint: locale.t("dc_hint_default") }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: locale.t("dc_details_label"); color: theme.textSub; font.pixelSize: 11; font.family: "Inter" }
                        OsmvTextField { Layout.fillWidth: true; placeholder: "Streaming on OBS"; text: osmvEngine.dc_details; onTextChanged: osmvEngine.dc_details = text }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: locale.t("dc_state_label"); color: theme.textSub; font.pixelSize: 11; font.family: "Inter" }
                        OsmvTextField { Layout.fillWidth: true; placeholder: "Playing games"; text: osmvEngine.dc_state; onTextChanged: osmvEngine.dc_state = text }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: theme.borderWhite; Layout.topMargin: 10; Layout.bottomMargin: 10 }

                // ── Pochette d'album ──────────────────────────────────────
                DiscordSection { title: locale.t("dc_section_large"); hint: locale.t("dc_hint_large") }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: locale.t("dc_large_key_label"); color: theme.textSub; font.pixelSize: 11; font.family: "Inter" }
                        OsmvTextField { Layout.fillWidth: true; placeholder: "osmv_logo"; text: osmvEngine.dc_large_key; onTextChanged: osmvEngine.dc_large_key = text }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: locale.t("dc_large_text_label"); color: theme.textSub; font.pixelSize: 11; font.family: "Inter" }
                        OsmvTextField { Layout.fillWidth: true; placeholder: "Hover text"; text: osmvEngine.dc_large_text; onTextChanged: osmvEngine.dc_large_text = text }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: theme.borderWhite; Layout.topMargin: 10; Layout.bottomMargin: 10 }

                // ── Icônes de statut ──────────────────────────────────────
                DiscordSection { title: locale.t("dc_section_status"); hint: locale.t("dc_hint_status") }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Clé Playing"; color: theme.colGreen; font.pixelSize: 11; font.family: "Inter" }
                        OsmvTextField { Layout.fillWidth: true; placeholder: "playing"; text: osmvEngine.dc_key_playing; onTextChanged: osmvEngine.dc_key_playing = text }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Clé Paused"; color: theme.colYellow; font.pixelSize: 11; font.family: "Inter" }
                        OsmvTextField { Layout.fillWidth: true; placeholder: "paused"; text: osmvEngine.dc_key_paused; onTextChanged: osmvEngine.dc_key_paused = text }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Clé Stopped"; color: theme.colRed; font.pixelSize: 11; font.family: "Inter" }
                        OsmvTextField { Layout.fillWidth: true; placeholder: "stopped"; text: osmvEngine.dc_key_stopped; onTextChanged: osmvEngine.dc_key_stopped = text }
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: theme.borderWhite; Layout.topMargin: 10; Layout.bottomMargin: 10 }

                // ── Petite image idle ─────────────────────────────────────
                DiscordSection { title: locale.t("dc_section_small"); hint: locale.t("dc_hint_small") }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: locale.t("dc_small_key_label"); color: theme.textSub; font.pixelSize: 11; font.family: "Inter" }
                        OsmvTextField { Layout.fillWidth: true; placeholder: "idle_icon"; text: osmvEngine.dc_small_key; onTextChanged: osmvEngine.dc_small_key = text }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: locale.t("dc_small_text_label"); color: theme.textSub; font.pixelSize: 11; font.family: "Inter" }
                        OsmvTextField { Layout.fillWidth: true; placeholder: "Hover text"; text: osmvEngine.dc_small_text; onTextChanged: osmvEngine.dc_small_text = text }
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
                        color: saveMouseArea.containsMouse ? theme.blurpleDim : theme.blurple
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
                                osmvEngine.save_discord_settings()
                                savedFeedback.visible = true
                                savedTimer.restart()
                            }
                        }
                    }

                    Text {
                        id: savedFeedback
                        text: locale.t("feedback_saved")
                        color: theme.colGreen
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
