import QtQuick
import QtQuick.Controls.Basic
import "style"

// Instance "Content" tab: mods, resource packs, shaders ... as a clean list with enable switches.
Rectangle {
    id: root
    color: Theme.bg

    property var types: []
    property int currentIndex: 0
    readonly property var current: types.length > 0 ? types[Math.min(currentIndex, types.length - 1)] : null
    readonly property var model: current ? current.model : null
    signal action(string action, string pageId)

    function refresh() {
        for (let i = 0; i < types.length; i++)
            types[i].model.refresh()
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
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                Repeater {
                    model: root.types
                    delegate: Chip {
                        required property var modelData
                        required property int index
                        text: modelData.label
                        count: modelData.model.count > 0 ? String(modelData.model.count) : ""
                        checked: root.currentIndex === index
                        onClicked: root.currentIndex = index
                    }
                }
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                WSearchField {
                    width: 240
                    placeholderText: qsTr("Search")
                    text: root.model ? root.model.filterText : ""
                    onTextChanged: if (root.model) root.model.filterText = text
                }
                WButton { icon: "plus"; text: qsTr("Add file"); onClicked: root.action("add", root.current.key) }
                WButton { kind: "primary"; icon: "download"; text: qsTr("Browse content"); onClicked: root.action("browse", root.current.key) }
            }
        }

        Item {
            id: summary
            anchors.top: toolbar.bottom
            anchors.topMargin: 14
            width: parent.width
            height: 32
            T {
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.textMuted
                text: root.model ? Theme.count(root.model.count, qsTr("1 item"), qsTr("%1 items")) + "  ·  " + qsTr("%1 enabled").arg(root.model.enabledCount) : ""
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                WButton { kind: "ghost"; compact: true; icon: "refresh"; text: qsTr("Check for updates"); visible: root.current && root.current.key !== "datapacks"; onClicked: root.action("updates", root.current.key) }
                WButton { kind: "ghost"; compact: true; icon: "folder"; text: qsTr("Open folder"); onClicked: if (root.model) root.model.openFolder() }
                WButton { kind: "ghost"; compact: true; icon: "sliders"; text: qsTr("Advanced"); onClicked: root.action("advanced", root.current.key) }
            }
        }

        Rectangle {
            id: listFrame
            anchors { top: summary.bottom; topMargin: 10; left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: Theme.pageMargin }
            radius: Theme.radiusCard
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
            clip: true

            ListView {
                id: list
                anchors.fill: parent
                anchors.margins: 6
                model: root.model
                boundsBehavior: Flickable.StopAtBounds
                spacing: 2
                ScrollBar.vertical: ScrollBar {
                    contentItem: Rectangle { implicitWidth: 6; radius: 3; color: "#2c3244" }
                }
                delegate: Rectangle {
                    id: row
                    required property int index
                    required property string name
                    required property string version
                    required property string fileName
                    required property bool isEnabled
                    required property string provider
                    required property bool hasIssues
                    width: list.width
                    height: 62
                    radius: 10
                    color: rowMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.fast } }

                    MouseArea { id: rowMouse; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }

                    Rectangle {
                        id: avatar
                        x: 12
                        anchors.verticalCenter: parent.verticalCenter
                        width: 40
                        height: 40
                        radius: 10
                        color: Theme.withAlpha(Theme.hashColor(row.name), 0.6)
                        opacity: row.isEnabled ? 1 : 0.4
                        T {
                            anchors.centerIn: parent
                            text: row.name.charAt(0).toUpperCase()
                            font.pixelSize: 17
                            font.weight: Font.Bold
                        }
                    }
                    Column {
                        anchors { left: avatar.right; leftMargin: 14; right: badges.left; rightMargin: 14; verticalCenter: parent.verticalCenter }
                        spacing: 3
                        opacity: row.isEnabled ? 1 : 0.5
                        T { width: parent.width; text: row.name; font.pixelSize: 14; font.weight: Font.DemiBold }
                        T {
                            width: parent.width
                            text: (row.version.length > 0 ? row.version + "  ·  " : "") + row.fileName
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontCaption
                        }
                    }
                    Row {
                        id: badges
                        anchors { right: controls.left; rightMargin: 16; verticalCenter: parent.verticalCenter }
                        spacing: 6
                        Badge { visible: row.hasIssues; text: qsTr("Incompatible"); tint: Theme.warning }
                        Badge { visible: row.provider.length > 0 && row.provider !== "Unknown"; text: row.provider; tint: Theme.sourceColor(row.provider) }
                    }
                    Row {
                        id: controls
                        anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        spacing: 12
                        IconButton {
                            anchors.verticalCenter: parent.verticalCenter
                            icon: "trash"
                            tip: qsTr("Remove")
                            opacity: rowMouse.containsMouse || hovered ? 1 : 0
                            property bool hovered: false
                            onClicked: root.model.remove(row.index)
                        }
                        WSwitch {
                            anchors.verticalCenter: parent.verticalCenter
                            checked: row.isEnabled
                            onToggled: (on) => root.model.setEnabled(row.index, on)
                        }
                    }
                }
            }

            EmptyState {
                anchors.centerIn: parent
                visible: !root.model || root.model.count === 0
                icon: "puzzle"
                title: root.model && root.model.filterText.length > 0 ? qsTr("Nothing matches") : qsTr("Nothing installed yet")
                text: root.model && root.model.filterText.length > 0 ? qsTr("Try a different search.")
                      : qsTr("Find mods, resource packs and shaders on Modrinth and CurseForge, or add files you already have.")
                WButton {
                    visible: !(root.model && root.model.filterText.length > 0)
                    kind: "primary"
                    icon: "download"
                    text: qsTr("Browse content")
                    onClicked: root.action("browse", root.current.key)
                }
            }
        }
    }
}
