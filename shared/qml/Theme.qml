// shared/qml/Theme.qml
// Palette de couleurs et constantes de design centralisées pour OSMV (Thème Sombre Statique).
import QtQuick 2.15

QtObject {
    // ── Thème statique sombre ────────────────────────────────────────────────
    readonly property bool isDark: true

    // ── Palette CSS réutilisable ─────────────────────────────────────────────
    readonly property color colorPrimary: "#5865f2" // Bleu principal (Discord Blurple)
    readonly property color colorPrimaryDim: "#3c3ca0" // Bleu assombri
    readonly property color colorPrimaryGlow: '#5830bbf2' // Lueur bleue (semi-transparente)
    readonly property color colorBorder: '#583ca0f2' // Bordure bleue (semi-transparente)
    readonly property color colorBorderGlow: '#58308ef2' // Halo de bordure bleu

    // ── Couleurs de fond ─────────────────────────────────────────────────────
    readonly property color bgDark: "#0c0c12"
    readonly property color bgCard: "#161624e6"
    readonly property color bgInput: "#12121e"

    // ── Accent principal et Discord Blurple (mappés sur la palette) ──────────
    readonly property color accent: colorPrimary
    readonly property color accentDim: colorPrimaryDim
    readonly property color accentGlow: colorPrimaryGlow
    readonly property color blurple: colorPrimary
    readonly property color blurpleDim: colorPrimaryDim
    readonly property color blurpleGlow: colorPrimaryGlow

    // ── Texte ────────────────────────────────────────────────────────────────
    readonly property color textMain: "#e6e6f5"
    readonly property color textSub: "#8c8ca5"
    readonly property color textDim: "#464664"

    // ── Statuts de lecture (mappés sur la palette bleue) ─────────────────────
    readonly property color colGreen: colorPrimary
    readonly property color colYellow: colorPrimary
    readonly property color colRed: "#ef4444"

    // ── Bordures (mappées sur la palette bleue) ──────────────────────────────
    readonly property color borderAccent: colorBorder
    readonly property color borderBlurple: colorBorder
    readonly property color borderWhite: colorBorder

    // ── Rayons ───────────────────────────────────────────────────────────────
    readonly property real radius: 14
    readonly property real radiusSmall: 8
    readonly property real radiusInput: 6

    // ── Timing des animations (ms) ───────────────────────────────────────────
    readonly property int durationFast: 150
    readonly property int durationMedium: 250
    readonly property int durationSlow: 400
}
