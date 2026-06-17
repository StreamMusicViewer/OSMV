// shared/qml/DiscordSection.qml
// En-tête de section pour l'onglet Discord RPC.

import QtQuick 2.15
import QtQuick.Layouts 1.15
import io.osmv 1.0

ColumnLayout {
    property string title: ""
    property string hint:  ""

    Layout.fillWidth:  true
    Layout.topMargin:  6
    Layout.leftMargin: 6
    spacing: 2

    Text {
        text:             title
        color:            theme.textMain
        font.pixelSize:   12
        font.weight:      Font.Bold
        font.family:      "Inter"
    }
    Text {
        visible:        hint.length > 0
        text:           hint
        color:          theme.textDim
        font.pixelSize: 11
        font.family:    "Inter"
    }
}
