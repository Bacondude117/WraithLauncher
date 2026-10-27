import QtQuick
import "style"

// Button. kind: "primary" (accent), "secondary" (raised), "ghost" (text only), "danger".
Rectangle {
    id: root
    property string text
    property string icon
    property string kind: "secondary"
    property bool compact: false
    signal clicked()

    readonly property bool hovered: mouse.containsMouse
    readonly property color fg: kind === "primary" ? Theme.accentText : kind === "danger" ? Theme.danger : Theme.text

    implicitHeight: compact ? 32 : 38
    implicitWidth: row.implicitWidth + (compact ? 22 : 30)
    radius: Theme.radiusButton
    opacity: enabled ? 1 : 0.45
    border.width: kind === "secondary" || kind === "danger" ? 1 : 0
    border.color: hovered ? Theme.borderStrong : Theme.border
    color: {
        if (kind === "primary")
            return mouse.pressed ? Qt.darker(Theme.accent, 1.12) : hovered ? Theme.accentHover : Theme.accent
        if (kind === "ghost")
            return mouse.pressed ? Theme.surfaceRaised : hovered ? Theme.surfaceHover : "transparent"
        return mouse.pressed ? Theme.surface : hovered ? "#2a3044" : Theme.surfaceRaised
    }
    Behavior on color { ColorAnimation { duration: Theme.fast } }
    scale: mouse.pressed ? 0.97 : 1
    Behavior on scale { NumberAnimation { duration: Theme.fast } }

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 8
        Icon {
            visible: root.icon.length > 0
            name: root.icon
            color: root.fg
            size: root.compact ? 15 : 17
            anchors.verticalCenter: parent.verticalCenter
        }
        T {
            visible: root.text.length > 0
            text: root.text
            color: root.fg
            font.pixelSize: root.compact ? 12 : 14
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
