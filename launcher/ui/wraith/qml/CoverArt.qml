import QtQuick
import "style"

// Cover art made from an instance icon: a blurred, darkened enlargement over a colour wash, clipped to rounded
// corners (optionally only the top ones). Rendered once in C++ by the image provider ("cover/"), so it works with
// Qt Quick's software renderer and costs no GPU memory.
Item {
    id: root
    property url source  // image://wraith/icon/<key>?r=<rev>
    property string seed: ""
    property real radius: Theme.radiusCard
    property bool roundBottom: true
    property real dim: 0.45

    readonly property string iconPart: {
        const s = source.toString()
        const prefix = "image://wraith/icon/"
        return s.startsWith(prefix) ? s.substring(prefix.length).split("?")[0] : ""
    }
    readonly property string revision: {
        const s = source.toString()
        return s.indexOf("?") >= 0 ? s.substring(s.indexOf("?") + 1) : ""
    }

    // the colour wash alone, until (or if no) icon is available
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        visible: cover.status !== Image.Ready
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: Qt.darker(Theme.hashColor(root.seed), 1.4) }
            GradientStop { position: 1; color: Qt.darker(Theme.hashColor(root.seed + "x"), 2.2) }
        }
    }
    Image {
        id: cover
        anchors.fill: parent
        cache: true  // the provider uses the icon list, which lives in the GUI thread: no asynchronous loading
        sourceSize: Qt.size(Math.max(1, Math.round(width)), Math.max(1, Math.round(height)))
        source: root.iconPart.length > 0 && width > 0 && height > 0
                ? "image://wraith/cover/" + root.iconPart + "?seed=" + encodeURIComponent(root.seed) + "&dim=" + root.dim
                  + "&radius=" + root.radius + "&bottom=" + (root.roundBottom ? 1 : 0) + "&" + root.revision
                : ""
    }
}
