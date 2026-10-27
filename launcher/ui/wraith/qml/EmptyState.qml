import QtQuick
import "style"

// Friendly placeholder: a big faint glyph, a title, a sentence and optional buttons.
Column {
    id: root
    property string icon: "ghost"
    property string title
    property string text
    default property alias actions: buttons.data

    spacing: 10
    width: Math.min(parent ? parent.width : 420, 420)

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        width: 84
        height: 84
        radius: 26
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        Icon { anchors.centerIn: parent; name: root.icon; size: 40; color: Theme.textFaint }
    }
    Item { width: 1; height: 6 }
    T {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: root.title
        font.pixelSize: 18
        font.weight: Font.DemiBold
    }
    T {
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: root.text
        color: Theme.textMuted
        wrapMode: Text.WordWrap
        elide: Text.ElideNone
    }
    Item { width: 1; height: 8 }
    Row {
        id: buttons
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10
    }
}
