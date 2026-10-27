import QtQuick
import "style"

// Filter pill; filled when checked. Optional colour dot (e.g. loader colour) and count.
Rectangle {
    id: root
    property string text
    property bool checked: false
    property color dot: "transparent"
    property string count
    signal clicked()

    implicitHeight: 32
    implicitWidth: row.implicitWidth + 26
    radius: 16
    color: checked ? Theme.withAlpha(Theme.accent, 0.16) : mouse.containsMouse ? Theme.surfaceHover : Theme.surface
    border.width: 1
    border.color: checked ? Theme.withAlpha(Theme.accent, 0.7) : Theme.border
    Behavior on color { ColorAnimation { duration: Theme.fast } }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 7
        Rectangle {
            visible: root.dot.a > 0
            width: 8; height: 8; radius: 4
            color: root.dot
            anchors.verticalCenter: parent.verticalCenter
        }
        T {
            text: root.text
            color: root.checked ? Theme.accent : mouse.containsMouse ? Theme.text : Theme.textMuted
            font.pixelSize: 13
            font.weight: root.checked ? Font.DemiBold : Font.Medium
        }
        T {
            visible: root.count.length > 0
            text: root.count
            color: Theme.textFaint
            font.pixelSize: 12
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
