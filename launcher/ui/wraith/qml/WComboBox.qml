import QtQuick
import QtQuick.Controls.Basic
import "style"

// Dropdown in the Wraith style. Set `label` for a muted prefix ("Sort: Name").
ComboBox {
    id: root
    property string label
    property string icon

    implicitHeight: 36
    implicitWidth: Math.max(150, content.implicitWidth + 48)
    font.pixelSize: 13

    background: Rectangle {
        radius: Theme.radiusButton
        color: root.pressed ? Theme.surfaceRaised : root.hovered ? Theme.surfaceHover : Theme.surface
        border.width: 1
        border.color: root.popup.visible || root.activeFocus ? Theme.accent : Theme.border
        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }

    contentItem: Row {
        id: content
        leftPadding: 12
        spacing: 7
        Icon {
            visible: root.icon.length > 0
            name: root.icon
            size: 15
            color: Theme.textMuted
            anchors.verticalCenter: parent.verticalCenter
        }
        T {
            visible: root.label.length > 0
            text: root.label
            color: Theme.textFaint
            anchors.verticalCenter: parent.verticalCenter
        }
        T {
            text: root.displayText
            font.weight: Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    indicator: Icon {
        x: root.width - width - 12
        anchors.verticalCenter: parent.verticalCenter
        name: "chevron-down"
        size: 14
        color: Theme.textMuted
        rotation: root.popup.visible ? 180 : 0
        Behavior on rotation { NumberAnimation { duration: Theme.fast } }
    }

    delegate: ItemDelegate {
        id: item
        required property int index
        required property var modelData
        width: root.popup.width - 12
        x: 6
        height: 34
        highlighted: root.highlightedIndex === index
        contentItem: T {
            text: root.textRole.length > 0 ? item.modelData[root.textRole] : item.modelData
            color: root.currentIndex === item.index ? Theme.accent : Theme.text
            font.weight: root.currentIndex === item.index ? Font.DemiBold : Font.Normal
        }
        background: Rectangle {
            radius: 7
            color: item.highlighted ? "#2f3650" : "transparent"
        }
    }

    popup: Popup {
        y: root.height + 6
        width: Math.max(root.width, 180)
        implicitHeight: Math.min(contentItem.implicitHeight + 12, 360)
        padding: 6
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            boundsBehavior: Flickable.StopAtBounds
        }
        background: Rectangle {
            radius: 12
            color: Theme.surfaceRaised
            border.width: 1
            border.color: Theme.borderStrong
        }
        enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.fast } }
    }
}
