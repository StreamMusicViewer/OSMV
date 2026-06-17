// shared/qml/OsmvSeparator.qml
// Séparateur horizontal pour l'interface OSMV.

import QtQuick 2.15
import QtQuick.Layouts 1.15

Item {
    Layout.fillWidth:   true
    Layout.topMargin:   10
    Layout.bottomMargin: 10
    height: 1

    Rectangle {
        anchors.fill: parent
        color: "#2a2a42"
    }
}
