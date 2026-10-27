pragma Singleton
import QtQuick

// Design tokens; keep in sync with WraithStyle.h (Wraith::Color) and DESIGN.md.
QtObject {
    readonly property color bg: "#0e1016"
    readonly property color sidebar: "#0a0c11"
    readonly property color surface: "#161922"
    readonly property color surfaceHover: "#1d2130"
    readonly property color surfaceRaised: "#222738"
    readonly property color border: "#262b3a"
    readonly property color borderStrong: "#353b50"
    readonly property color text: "#e8eaf2"
    readonly property color textMuted: "#9aa1b5"
    readonly property color textFaint: "#636a80"
    readonly property color accent: "#7fdbea"
    readonly property color accentHover: "#9de6f1"
    readonly property color accentText: "#0b1418"
    readonly property color violet: "#a58cff"
    readonly property color success: "#4fd18b"
    readonly property color warning: "#f2c45a"
    readonly property color danger: "#ff6b7d"

    readonly property int radiusSmall: 6
    readonly property int radiusButton: 10
    readonly property int radiusCard: 14
    readonly property int radiusLarge: 20
    readonly property int pageMargin: 32

    readonly property int fontTitle: 26
    readonly property int fontSection: 17
    readonly property int fontCardTitle: 15
    readonly property int fontBody: 13
    readonly property int fontCaption: 12

    readonly property int fast: 140
    readonly property int medium: 220

    function loaderColor(loader) {
        switch (loader) {
        case "Fabric": return "#dbb68f"
        case "Quilt": return "#c58cff"
        case "Forge": return "#7fa7ff"
        case "NeoForge": return "#f5935c"
        case "Vanilla": return "#7fd88f"
        default: return "#9aa1b5"
        }
    }

    function sourceColor(source) {
        switch (source) {
        case "Modrinth": return "#4fd18b"
        case "CurseForge": return "#f28c55"
        case "FTB": return "#7fa7ff"
        case "ATLauncher": return "#7fdbea"
        case "Technic": return "#f2c45a"
        default: return "#9aa1b5"
        }
    }

    // stable colour for an instance without real cover art
    function hashColor(name) {
        let h = 0
        for (let i = 0; i < name.length; i++)
            h = (h * 31 + name.charCodeAt(i)) % 360
        return Qt.hsla(h / 360, 0.45, 0.32, 1)
    }

    // "1 instance" / "4 instances" (explicit forms until translations provide plurals)
    function count(n, one, many) {
        return n === 1 ? one : many.arg(n)
    }

    function withAlpha(c, a) {
        return Qt.rgba(c.r, c.g, c.b, a)
    }
}
