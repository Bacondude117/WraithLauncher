import QtQuick
import "style"

// On/off switch.
Rectangle {
    id: root
    property bool checked: false
    signal toggled(bool checked)

    width: 42
    height: 24
    radius: 12
    color: checked ? Theme.accent : mouse.containsMouse ? "#3a4157" : Theme.surfaceRaised
    border.width: checked ? 0 : 1
    border.color: Theme.borderStrong
    Behavior on color { ColorAnimation { duration: Theme.fast } }

    Rectangle {
        width: 16
        height: 16
        radius: 8
        y: 4
        x: root.checked ? root.width - width - 4 : 4
        color: root.checked ? Theme.accentText : Theme.textMuted
        Behavior on x { NumberAnimation { duration: Theme.fast; easing.type: Easing.OutCubic } }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.toggled(!root.checked)
    }
}
