import QtQuick
import QtQuick.Controls.Basic
import "style"

// Rounded search input with a leading glyph and a clear button.
TextField {
    id: root
    implicitWidth: 300
    implicitHeight: 40
    leftPadding: 40
    rightPadding: text.length > 0 ? 36 : 14
    color: Theme.text
    placeholderTextColor: Theme.textFaint
    selectionColor: "#2d5560"
    selectedTextColor: Theme.text
    font.pixelSize: 13
    renderType: Text.NativeRendering
    verticalAlignment: TextInput.AlignVCenter

    background: Rectangle {
        radius: Theme.radiusButton
        color: root.activeFocus ? Theme.surfaceRaised : Theme.surface
        border.width: 1
        border.color: root.activeFocus ? Theme.accent : hover.hovered ? Theme.borderStrong : Theme.border
        Behavior on border.color { ColorAnimation { duration: Theme.fast } }
        Icon {
            x: 13
            anchors.verticalCenter: parent.verticalCenter
            name: "search"
            size: 16
            color: root.activeFocus ? Theme.accent : Theme.textFaint
        }
    }

    HoverHandler { id: hover; cursorShape: Qt.IBeamCursor }

    IconButton {
        visible: root.text.length > 0
        anchors.right: parent.right
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        size: 26
        icon: "close"
        onClicked: root.clear()
    }

    Keys.onEscapePressed: root.clear()
}
