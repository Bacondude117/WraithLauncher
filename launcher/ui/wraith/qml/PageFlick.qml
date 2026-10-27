import QtQuick
import QtQuick.Controls.Basic
import "style"

// Vertically scrolling page body with Wraith page margins and a slim scrollbar; plays the enter animation.
Flickable {
    id: root
    default property alias content: column.data
    property alias spacing: column.spacing
    property int maxContentWidth: 1400

    contentWidth: width
    contentHeight: column.implicitHeight + Theme.pageMargin * 2
    boundsBehavior: Flickable.StopAtBounds
    clip: true

    function enter() {
        enterAnim.restart()
    }

    Column {
        id: column
        x: Math.max(Theme.pageMargin, (root.width - root.maxContentWidth) / 2)
        y: Theme.pageMargin
        width: Math.min(root.width - Theme.pageMargin * 2, root.maxContentWidth)
        spacing: 28
    }

    ParallelAnimation {
        id: enterAnim
        NumberAnimation { target: column; property: "opacity"; from: 0; to: 1; duration: Theme.medium; easing.type: Easing.OutCubic }
        NumberAnimation { target: column; property: "y"; from: Theme.pageMargin + 12; to: Theme.pageMargin; duration: Theme.medium; easing.type: Easing.OutCubic }
    }

    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
        contentItem: Rectangle {
            implicitWidth: 6
            radius: 3
            color: parent.pressed ? "#4a5270" : "#2c3244"
        }
    }
}
