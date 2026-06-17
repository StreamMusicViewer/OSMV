// shared/qml/main.qml
// Fenêtre principale OSMV — Dark glassmorphism, barre d'onglets animée,
// SwipeView avec transitions de glissement entre "Now Playing" et "Discord RPC".

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import io.osmv 1.0

Window {
    id: root
    title: "OBS Stream Music Viewer"
    width: 500
    height: 340
    minimumWidth:  460
    minimumHeight: 300

    Theme {
        id: theme
    }

    OsmvLocale {
        id: locale
    }

    // Arrière-plan s'adaptant au thème du système
    color: theme.bgDark
    visible: true

    // L'instance OsmvEngine QObject créée en Rust, accessible dans toute la hiérarchie QML
    OsmvEngine {
        id: osmvEngine
    }

    // ── Contenu principal ─────────────────────────────────────────────────────
    Column {
        anchors.fill: parent
        spacing: 0

        // ── Barre d'onglets ───────────────────────────────────────────────
        Rectangle {
            id: tabBar
            width: parent.width
            height: 46
            color: theme.bgDark

            Row {
                anchors.fill: parent
                spacing: 0

                // Espaceur gauche
                Item { width: 14; height: 1; anchors.verticalCenter: parent.verticalCenter }

                // ── Onglet Now Playing ────────────────────────────────────────
                OsmvTabButton {
                    id: tabNP
                    targetIndex: 0
                    label: locale.t("tab_now_playing")
                    icon: "♫"
                    width: 110
                    currentIndex: swipeView.currentIndex
                    onTabClicked: swipeView.currentIndex = 0
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: 4; height: 1; anchors.verticalCenter: parent.verticalCenter }

                // ── Onglet Discord RPC ────────────────────────────────────────
                OsmvTabButton {
                    id: tabDC
                    targetIndex: 1
                    label: locale.t("tab_discord_rpc")
                    icon: "⬡"
                    width: 110
                    currentIndex: swipeView.currentIndex
                    onTabClicked: swipeView.currentIndex = 1
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: 4; height: 1; anchors.verticalCenter: parent.verticalCenter }

                // ── Onglet Paramètres ─────────────────────────────────────────
                OsmvTabButton {
                    id: tabSettings
                    targetIndex: 2
                    label: locale.t("tab_settings")
                    icon: "⚙"
                    width: 110
                    currentIndex: swipeView.currentIndex
                    onTabClicked: swipeView.currentIndex = 2
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: 4; height: 1; anchors.verticalCenter: parent.verticalCenter }

                // ── Onglet Aide ───────────────────────────────────────────────
                OsmvTabButton {
                    id: tabHelp
                    targetIndex: 3
                    label: locale.t("tab_help")
                    icon: "🛈"
                    width: 80
                    currentIndex: swipeView.currentIndex
                    onTabClicked: swipeView.currentIndex = 3
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Séparateur de bas de la barre
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: theme.borderAccent
            }
        }

        // ── Vues paginées avec glissement ─────────────────────────────────
        SwipeView {
            id: swipeView
            width: parent.width
            height: parent.height - tabBar.height
            clip: true
            interactive: true    // Permet le swipe tactile / souris

            // Transition de glissement fluide entre les vues
            contentItem: ListView {
                id: listView
                model: swipeView.contentModel
                interactive: swipeView.interactive
                currentIndex: swipeView.currentIndex
                spacing: swipeView.spacing
                orientation: ListView.Horizontal
                snapMode: ListView.SnapOneItem
                highlightRangeMode: ListView.StrictlyEnforceRange
                preferredHighlightBegin: 0
                preferredHighlightEnd: width
                highlightMoveDuration: 280
                highlightMoveVelocity: -1

                // Easing du glissement
                Behavior on contentX {
                    SmoothedAnimation {
                        velocity: -1
                        duration: 280
                        easing.type: Easing.OutCubic
                    }
                }
            }

            NowPlayingView { }
            DiscordView    { }
            OsmvSettingsView { }
            OsmvHelpView   { }
        }
    }
}


