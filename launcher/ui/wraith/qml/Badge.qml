import QtQuick
import "style"

// Small tinted label, e.g. loader or version.
Rectangle {
    id: root
    property string text
    property color tint: Theme.textMuted
    property bool solid: false

    implicitWidth: label.implicitWidth + 14
    implicitHeight: 22
    radius: Theme.radiusSmall
    color: solid ? tint : Theme.withAlpha(tint, 0.14)

    T {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.solid ? Theme.accentText : root.tint
        font.pixelSize: 12
        font.weight: Font.DemiBold
    }
}
