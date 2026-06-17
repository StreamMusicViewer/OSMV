// shared/qml/Theme.qml
// Palette de couleurs et constantes de design centralisées pour OSMV (Thème Sombre Statique).
import QtQuick 2.15

QtObject {
    // ── Thème statique sombre ────────────────────────────────────────────────
    readonly property bool isDark: true

    // ── Couleurs de fond ─────────────────────────────────────────────────────
    readonly property color bgDark:   "#0c0c12"
    readonly property color bgCard:   "#161624e6"
    readonly property color bgInput:  "#12121e"

    // ── Accent principal (Bleu Discord) ──────────────────────────────────────
    readonly property color accent:      "#5865f2"
    readonly property color accentDim:   "#3c3ca0"
    readonly property color accentGlow:  "#5865f230"

    // ── Discord Blurple ──────────────────────────────────────────────────────
    readonly property color blurple:     "#5865f2"
    readonly property color blurpleDim:  "#3c3ca0"
    readonly property color blurpleGlow: "#5865f230"

    // ── Texte ────────────────────────────────────────────────────────────────
    readonly property color textMain: "#e6e6f5"
    readonly property color textSub:  "#8c8ca5"
    readonly property color textDim:  "#464664"

    // ── Statuts de lecture ───────────────────────────────────────────────────
    readonly property color colGreen:  "#22c55e"
    readonly property color colYellow: "#eab308"
    readonly property color colRed:    "#ef4444"

    // ── Bordures des cartes ──────────────────────────────────────────────────
    readonly property color borderAccent:  "#5865f23c"
    readonly property color borderBlurple: "#5865f23c"
    readonly property color borderWhite:   "#ffffff14"

    // ── Rayons ───────────────────────────────────────────────────────────────
    readonly property real radius:       14
    readonly property real radiusSmall:  8
    readonly property real radiusInput:  6

    // ── Timing des animations (ms) ───────────────────────────────────────────
    readonly property int durationFast:   150
    readonly property int durationMedium: 250
    readonly property int durationSlow:   400
}
