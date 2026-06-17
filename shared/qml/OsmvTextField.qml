// shared/qml/OsmvTextField.qml
// Champ de saisie stylisé avec animation de focus pour l'interface OSMV.

import QtQuick 2.15
import QtQuick.Controls 2.15
import io.osmv 1.0

TextField {
    id: root
    property string placeholder: ""

    placeholderText:      placeholder
    color:                theme.textMain
    font.pixelSize:       12
    font.family:          "Inter"
    height:               32
    placeholderTextColor: theme.textDim
    leftPadding:          10
    rightPadding:         10

    background: Rectangle {
        color:         root.activeFocus ? (theme.isDark ? "#1a1a30" : "#e5e7eb") : theme.bgInput
        radius:        theme.radiusInput
        border.color:  root.activeFocus ? theme.accent : theme.borderAccent
        border.width:  1
        Behavior on border.color { ColorAnimation { duration: 150 } }
        Behavior on color        { ColorAnimation { duration: 150 } }
    }
}
