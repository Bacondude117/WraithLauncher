import QtQuick
import "style"

// A tinted line glyph from launcher/ui/wraith/icons.
Image {
    property string name
    property color color: Theme.text
    property int size: 18

    width: size
    height: size
    source: name.length > 0 ? backend.glyph(name, color.toString()) : ""
    sourceSize: Qt.size(size * 2, size * 2)
    smooth: true
    mipmap: true
    fillMode: Image.PreserveAspectFit
}
