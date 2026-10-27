import QtQuick
import "style"

// Home quick-action tile.
Rectangle {
    id: root
    property string icon
    property string title
    property string subtitle
    property color tint: Theme.accent
    signal clicked()

    implicitHeight: 76
    radius: Theme.radiusCard
    color: mouse.containsMouse ? Theme.surfaceHover : Theme.surface
    border.width: 1
    border.color: mouse.containsMouse ? Theme.borderStrong : Theme.border
    Behavior on color { ColorAnimation { duration: Theme.fast } }

    Rectangle {
        id: badge
        x: 16
        anchors.verticalCenter: parent.verticalCenter
        width: 42
        height: 42
        radius: 12
        color: Theme.withAlpha(root.tint, 0.14)
        Icon { anchors.centerIn: parent; name: root.icon; size: 20; color: root.tint }
    }
    Column {
        anchors { left: badge.right; leftMargin: 14; right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
        spacing: 2
        T { width: parent.width; text: root.title; font.pixelSize: 14; font.weight: Font.DemiBold }
        T { width: parent.width; text: root.subtitle; color: Theme.textMuted; font.pixelSize: Theme.fontCaption }
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
