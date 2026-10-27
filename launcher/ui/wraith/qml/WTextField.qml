import QtQuick
import QtQuick.Controls.Basic
import "style"

// Single-line text input in the Wraith style.
TextField {
    id: root
    implicitHeight: 40
    leftPadding: 14
    rightPadding: 14
    color: Theme.text
    placeholderTextColor: Theme.textFaint
    selectionColor: "#2d5560"
    selectedTextColor: Theme.text
    font.pixelSize: 13
    renderType: Text.NativeRendering
    verticalAlignment: TextInput.AlignVCenter
    background: Rectangle {
        radius: Theme.radiusButton
        color: Theme.surface
        border.width: 1
        border.color: root.activeFocus ? Theme.accent : root.hovered ? Theme.borderStrong : Theme.border
        Behavior on border.color { ColorAnimation { duration: Theme.fast } }
    }
}
