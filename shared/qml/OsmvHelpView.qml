// shared/qml/OsmvHelpView.qml
// Panneau À Propos / Aide pour l'interface OSMV avec logo stylisé animé.

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

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width - 32
            spacing: 12
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            // ── Logo du Projet (en haut, centré) ──────────────────────────────
            Item {
                width: 100
                height: 100
                Layout.alignment: Qt.AlignHCenter

                // Fond lumineux derrière le logo
                Rectangle {
                    anchors.centerIn: parent
                    width: 86; height: 86
                    radius: 43
                    color: "transparent"
                    border.color: theme.accentGlow
                    border.width: 4

                    // Effet de pulsation douce
                    SequentialAnimation on scale {
                        loops: Animation.Infinite
                        running: true
                        NumberAnimation { to: 1.10; duration: 2000; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 1.00; duration: 2000; easing.type: Easing.InOutSine }
                    }
                }

                // Logo OSMV depuis l'image .ico
                Image {
                    anchors.centerIn: parent
                    width: 76; height: 76
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                    source: osmvEngine.logo_path.length > 0 ? ("file://" + osmvEngine.logo_path) : ""
                }
            }

            // ── Informations (juste en dessous, centrées) ───────────────────
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                Layout.alignment: Qt.AlignHCenter

                Text {
                    text: locale.t("help_title")
                    color: theme.textMain
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    font.family: "Inter"
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: locale.t("help_version") + " : 2.0.0"
                    color: theme.accent
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    font.family: "Inter"
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                // Séparateur centré
                Rectangle {
                    implicitWidth: 140
                    height: 1
                    color: theme.borderAccent
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    text: locale.t("help_desc")
                    color: theme.textSub
                    font.pixelSize: 11
                    font.family: "Inter"
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                    maximumLineCount: 3
                }

                Item { height: 4 }

                Text {
                    text: locale.t("help_author")
                    color: theme.textDim
                    font.pixelSize: 10
                    font.family: "Inter"
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: locale.t("help_license")
                    color: theme.textDim
                    font.pixelSize: 10
                    font.family: "Inter"
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
