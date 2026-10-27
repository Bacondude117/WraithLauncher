import QtQuick
import "style"

// Sidebar entry: icon + label, filled pill and accent bar when active.
Item {
    id: root
    property string icon
    property string text
    property bool active: false
    property string badge
    signal clicked()

    width: parent ? parent.width : 200
    height: 42

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        radius: Theme.radiusButton
        color: root.active ? Theme.withAlpha(Theme.accent, 0.12) : mouse.containsMouse ? Theme.surface : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.fast } }

        Rectangle {
            width: 3
            height: root.active ? 18 : 0
            radius: 2
            color: Theme.accent
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            Behavior on height { NumberAnimation { duration: Theme.medium; easing.type: Easing.OutCubic } }
        }

        Icon {
            id: glyph
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            name: root.icon
            size: 19
            color: root.active ? Theme.accent : mouse.containsMouse ? Theme.text : Theme.textMuted
        }
        T {
            anchors.left: glyph.right
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            font.pixelSize: 14
            font.weight: root.active ? Font.DemiBold : Font.Medium
            color: root.active ? Theme.text : mouse.containsMouse ? Theme.text : Theme.textMuted
        }
        Rectangle {
            visible: root.badge.length > 0
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            height: 20
            width: Math.max(20, badgeText.implicitWidth + 12)
            radius: 10
            color: Theme.withAlpha(Theme.success, 0.16)
            T {
                id: badgeText
                anchors.centerIn: parent
                text: root.badge
                color: Theme.success
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
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
