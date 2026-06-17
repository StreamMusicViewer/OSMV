// shared/qml/OsmvSwitch.qml
// Toggle switch réutilisable pour l'interface OSMV.

import QtQuick 2.15
import io.osmv 1.0

Item {
    id: root
    property string label:   ""
    property bool   checked: false

    signal clicked()

    implicitHeight: 24
    implicitWidth:  200

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.clicked()
        }
    }

    Row {
        spacing: 8
        anchors.verticalCenter: parent.verticalCenter

        // Piste du toggle
        Rectangle {
            id: track
            width: 36; height: 20
            radius: 10
            color: root.checked ? theme.accent : theme.bgInput
            border.color: root.checked ? theme.accent : theme.borderWhite
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter
            Behavior on color { ColorAnimation { duration: 200 } }

            // Poignée du toggle
            Rectangle {
                x: root.checked ? parent.width - width - 3 : 3
                anchors.verticalCenter: parent.verticalCenter
                width: 14; height: 14
                radius: 7
                color: root.checked ? "white" : theme.textSub
                Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                Behavior on color { ColorAnimation { duration: 200 } }
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.label
            color: theme.textSub
            font.pixelSize: 12
            font.family: "Inter"
        }
    }
}
