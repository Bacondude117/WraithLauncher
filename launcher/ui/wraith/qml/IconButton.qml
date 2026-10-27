import QtQuick
import QtQuick.Controls.Basic
import "style"

// Round icon-only button with a tooltip.
Rectangle {
    id: root
    property string icon
    property string tip
    property color iconColor: Theme.textMuted
    property int size: 34
    property bool filled: false
    signal clicked()

    width: size
    height: size
    radius: size / 2
    color: mouse.pressed ? Theme.surfaceRaised : mouse.containsMouse ? Theme.surfaceHover : filled ? Theme.surfaceRaised : "transparent"
    Behavior on color { ColorAnimation { duration: Theme.fast } }

    Icon {
        anchors.centerIn: parent
        name: root.icon
        size: Math.round(root.size * 0.5)
        color: mouse.containsMouse ? Theme.text : root.iconColor
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: tip.length > 0 && mouse.containsMouse
    ToolTip.delay: 500
    ToolTip.text: tip
}
