import QtQuick
import QtQuick.Controls.Basic
import "style"

// Instance "Worlds" tab: world cards with quick-join Play; the full world tools are under Advanced.
Rectangle {
    id: root
    color: Theme.bg

    property var worlds: null
    property string instanceId
    signal advanced()

    function refresh() {
        if (worlds)
            worlds.refresh()
    }

    Item {
        anchors.fill: parent
        anchors.margins: Theme.pageMargin
        anchors.topMargin: 20
        anchors.bottomMargin: 0

        Item {
            id: toolbar
            width: parent.width
            height: 40
            T {
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.textMuted
                text: root.worlds ? Theme.count(root.worlds.count, qsTr("1 world"), qsTr("%1 worlds")) : ""
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                WButton { icon: "sliders"; text: qsTr("Advanced tools"); onClicked: root.advanced() }
            }
        }

        GridView {
            id: grid
            anchors { top: toolbar.bottom; topMargin: 16; left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.leftMargin: -8
            anchors.rightMargin: -8
            clip: true
            model: root.worlds
            boundsBehavior: Flickable.StopAtBounds
            readonly property int columns: Math.max(1, Math.floor(width / 340))
            cellWidth: Math.floor(width / columns)
            cellHeight: 112
            bottomMargin: Theme.pageMargin
            delegate: Item {
                id: cell
                required property int index
                required property string name
                required property string gameMode
                required property string lastPlayedText
                required property string seed
                required property string size
                required property string icon
                width: grid.cellWidth
                height: grid.cellHeight

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 8
                    radius: Theme.radiusCard
                    color: cardMouse.containsMouse ? Theme.surfaceHover : Theme.surface
                    border.width: 1
                    border.color: cardMouse.containsMouse ? Theme.borderStrong : Theme.border
                    Behavior on color { ColorAnimation { duration: Theme.fast } }

                    MouseArea { id: cardMouse; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }

                    Rectangle {
                        id: thumb
                        x: 14
                        anchors.verticalCenter: parent.verticalCenter
                        width: 64
                        height: 64
                        radius: 12
                        color: Theme.surfaceRaised
                        clip: true
                        Image {
                            anchors.fill: parent
                            source: cell.icon
                            visible: cell.icon.length > 0
                            fillMode: Image.PreserveAspectCrop
                            smooth: false
                        }
                        Icon { anchors.centerIn: parent; visible: cell.icon.length === 0; name: "map"; size: 26; color: Theme.textFaint }
                    }
                    Column {
                        anchors { left: thumb.right; leftMargin: 14; right: playBox.left; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        spacing: 5
                        T { width: parent.width; text: cell.name; font.pixelSize: 14; font.weight: Font.DemiBold }
                        Row {
                            spacing: 6
                            Badge { visible: cell.gameMode.length > 0; text: cell.gameMode; tint: Theme.violet }
                            Badge { text: cell.size; tint: Theme.textMuted }
                        }
                        T { width: parent.width; text: cell.lastPlayedText; color: Theme.textFaint; font.pixelSize: Theme.fontCaption }
                    }
                    Column {
                        id: playBox
                        anchors { right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
                        spacing: 6
                        PlayButton { id: play; size: 40; onClicked: root.worlds.play(cell.index) }
                    }
                    Row {
                        anchors { right: parent.right; rightMargin: 62; top: parent.top; topMargin: 8 }
                        spacing: 2
                        opacity: cardMouse.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: Theme.fast } }
                        IconButton { size: 28; icon: "folder"; tip: qsTr("Open folder"); onClicked: root.worlds.openFolder(cell.index) }
                        IconButton { size: 28; icon: "copy"; tip: qsTr("Copy seed"); onClicked: { root.worlds.copySeed(cell.index); backend.toast(qsTr("Seed copied"), "success") } }
                    }
                }
            }
        }

        EmptyState {
            anchors.centerIn: grid
            visible: !root.worlds || root.worlds.count === 0
            icon: "map"
            title: qsTr("No worlds yet")
            text: qsTr("Worlds you create in game show up here. You can also add existing world folders or zips.")
            WButton { icon: "sliders"; text: qsTr("Add a world"); onClicked: root.advanced() }
        }
    }
}
