// shared/qml/TabButton.qml
// Bouton d'onglet animé pour la barre de navigation OSMV.

import QtQuick 2.15
import QtQuick.Controls 2.15
import io.osmv 1.0

Item {
    id: tabBtn

    property int    targetIndex:  0
    property string label:        ""
    property string icon:         ""
    property int    currentIndex: 0
    signal tabClicked()

    readonly property bool active: currentIndex === targetIndex

    implicitWidth:  130
    implicitHeight: 38

    // ── Fond du bouton ────────────────────────────────────────────────────────
    Rectangle {
        id: btnBg
        anchors.fill: parent
        radius: theme.radiusSmall
        color: {
            if (tabBtn.active)           return tabBtn.targetIndex === 0 ? theme.accent : theme.blurple
            if (hoverArea.containsMouse) return theme.bgInput
            return "transparent"
        }
        Behavior on color { ColorAnimation { duration: 180 } }

        // Halo de soulignement sous le bouton actif
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: -3
            width: parent.width * 0.7
            height: 3
            radius: 2
            color: tabBtn.targetIndex === 0 ? theme.accent : theme.blurple
            opacity: tabBtn.active ? 0.8 : 0
            Behavior on opacity { NumberAnimation { duration: 250 } }
        }
    }

    // ── Label avec icône ──────────────────────────────────────────────────────
    Row {
        anchors.centerIn: parent
        spacing: 5

        Text {
            text: tabBtn.icon
            color: tabBtn.active ? "white" : theme.textSub
            font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color { ColorAnimation { duration: 180 } }
        }
        Text {
            text: tabBtn.label
            color: tabBtn.active ? "white" : theme.textSub
            font.pixelSize: 12
            font.weight: Font.Bold
            font.family: "Inter"
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color { ColorAnimation { duration: 180 } }
        }
    }

    // Micro-animation de scale au clic
    scale: hoverArea.pressed ? 0.95 : 1.0
    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: tabBtn.tabClicked()
    }
}
