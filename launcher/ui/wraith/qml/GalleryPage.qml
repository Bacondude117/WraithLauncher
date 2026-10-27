import QtQuick
import QtQuick.Controls.Basic
import "style"

// Every screenshot from every instance, newest first, with a large preview.
Rectangle {
    id: root
    color: Theme.bg
    property int previewRow: -1

    function pageShown() { galleryModel.refresh() }
    function preview(row) { previewRow = Number(row) }  // used by the UI test script

    Item {
        anchors.fill: parent
        anchors.margins: Theme.pageMargin
        anchors.bottomMargin: 0

        Item {
            id: header
            width: parent.width
            height: 48
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                T { text: qsTr("Screenshots"); font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
                T { color: Theme.textMuted; text: Theme.count(galleryModel.count, qsTr("1 screenshot"), qsTr("%1 screenshots")) }
            }
            WComboBox {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: galleryModel.instanceNames.length > 1
                icon: "library"
                model: [qsTr("All instances")].concat(galleryModel.instanceNames)
                onActivated: (index) => galleryModel.instanceFilter = index === 0 ? "" : galleryModel.instanceNames[index - 1]
            }
        }

        GridView {
            id: grid
            anchors { top: header.bottom; topMargin: 22; left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.leftMargin: -6
            anchors.rightMargin: -6
            clip: true
            model: galleryModel
            boundsBehavior: Flickable.StopAtBounds
            bottomMargin: Theme.pageMargin
            readonly property int columns: Math.max(1, Math.floor(width / 300))
            cellWidth: Math.floor(width / columns)
            cellHeight: Math.round(cellWidth * 9 / 16) + 52
            ScrollBar.vertical: ScrollBar { contentItem: Rectangle { implicitWidth: 6; radius: 3; color: "#2c3244" } }
            delegate: Item {
                id: cell
                required property int index
                required property url url
                required property string instance
                required property string date
                width: grid.cellWidth
                height: grid.cellHeight
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 6
                    radius: Theme.radiusCard
                    color: Theme.surface
                    border.width: 1
                    border.color: shotMouse.containsMouse ? Theme.borderStrong : Theme.border
                    clip: true
                    Image {
                        id: thumb
                        anchors { left: parent.left; right: parent.right; top: parent.top; margins: 1 }
                        height: parent.height - 46
                        source: cell.url
                        sourceSize.width: 480
                        asynchronous: true
                        fillMode: Image.PreserveAspectCrop
                    }
                    Column {
                        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 10 }
                        T { width: parent.width; text: cell.instance; font.weight: Font.Medium }
                        T { text: cell.date; color: Theme.textFaint; font.pixelSize: Theme.fontCaption }
                    }
                    MouseArea { id: shotMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.previewRow = cell.index }
                }
            }
        }
        EmptyState {
            anchors.centerIn: grid
            visible: galleryModel.count === 0
            icon: "image"
            title: qsTr("No screenshots yet")
            text: qsTr("Press F2 in game to take one. They show up here from every instance.")
        }
    }

    // large preview
    Rectangle {
        anchors.fill: parent
        visible: root.previewRow >= 0
        color: Theme.withAlpha("#05060a", 0.92)
        MouseArea { anchors.fill: parent; onClicked: root.previewRow = -1 }
        Image {
            anchors { fill: parent; margins: 48; bottomMargin: 96 }
            source: root.previewRow >= 0 && root.previewRow < grid.count ? grid.itemAtIndex(root.previewRow).url : ""
            asynchronous: true
            fillMode: Image.PreserveAspectFit
        }
        Row {
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 32 }
            spacing: 10
            WButton { icon: "back"; text: qsTr("Previous"); onClicked: root.previewRow = Math.max(0, root.previewRow - 1) }
            WButton { icon: "external"; text: qsTr("Open"); onClicked: galleryModel.openFile(root.previewRow) }
            WButton { icon: "copy"; text: qsTr("Copy"); onClicked: galleryModel.copyImage(root.previewRow) }
            WButton { icon: "folder"; text: qsTr("Show in folder"); onClicked: galleryModel.showInFolder(root.previewRow) }
            WButton { kind: "danger"; icon: "trash"; text: qsTr("Delete..."); onClicked: { galleryModel.remove(root.previewRow); root.previewRow = -1 } }
            WButton { text: qsTr("Next"); onClicked: root.previewRow = Math.min(grid.count - 1, root.previewRow + 1) }
            IconButton { anchors.verticalCenter: parent.verticalCenter; icon: "close"; tip: qsTr("Close"); onClicked: root.previewRow = -1 }
        }
    }
}
