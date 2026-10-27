import QtQuick
import QtQuick.Controls.Basic
import "style"

// Round Play button that turns into a red Stop button while the instance runs.
Rectangle {
    id: root
    property bool running: false
    property int size: 44
    signal clicked()

    width: size
    height: size
    radius: size / 2
    color: running ? (mouse.containsMouse ? "#ff8796" : Theme.danger) : (mouse.containsMouse ? Theme.accentHover : Theme.accent)
    Behavior on color { ColorAnimation { duration: Theme.fast } }
    scale: mouse.pressed ? 0.92 : mouse.containsMouse ? 1.06 : 1
    Behavior on scale { NumberAnimation { duration: Theme.fast; easing.type: Easing.OutCubic } }

    Icon {
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: root.running ? 0 : root.size * 0.04
        name: root.running ? "stop" : "play"
        color: Theme.accentText
        size: Math.round(root.size * 0.42)
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: mouse.containsMouse
    ToolTip.delay: 600
    ToolTip.text: running ? qsTr("Stop") : qsTr("Play")
}
