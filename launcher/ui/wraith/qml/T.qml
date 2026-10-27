import QtQuick
import "style"

// Text with the Wraith defaults.
Text {
    color: Theme.text
    font.pixelSize: Theme.fontBody
    renderType: Text.NativeRendering
    elide: Text.ElideRight
    verticalAlignment: Text.AlignVCenter
}
