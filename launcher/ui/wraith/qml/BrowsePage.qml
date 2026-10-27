import QtQuick
import "style"

Rectangle {
    color: Theme.bg
    function pageShown() {}
    function search(q) {}
    T { anchors.centerIn: parent; text: "BrowsePage"; color: Theme.textMuted }
}
