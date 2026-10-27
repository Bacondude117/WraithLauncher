import QtQuick
import "style"

// Library: every instance as a card, with search, loader chips, group filter and sort.
Rectangle {
    id: root
    color: Theme.bg

    readonly property var model: backend.instances
    readonly property var loaders: ["", "Vanilla", "Fabric", "Quilt", "Forge", "NeoForge", "Other"]
    readonly property var sortModes: [
        { key: "Name", label: qsTr("Name") },
        { key: "LastLaunch", label: qsTr("Last played") },
        { key: "Playtime", label: qsTr("Playtime") }
    ]

    function pageShown() { enterAnim.restart() }
    function setSearch(text) { search.text = text }
    function setLoader(loader) { model.loaderFilter = loader }
    function focusGrid(unused) { grid.forceActiveFocus() }

    Shortcut { sequences: [StandardKey.Find]; onActivated: search.forceActiveFocus() }

    Image {
        anchors.fill: parent
        source: backend.catImage
        visible: source.toString().length > 0
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignRight
        verticalAlignment: Image.AlignBottom
        opacity: 0.18
    }

    DropArea {
        anchors.fill: parent
        onDropped: (drop) => { if (drop.hasUrls) backend.dropUrls(drop.urls) }
    }

    Item {
        id: content
        anchors.fill: parent
        anchors.margins: Theme.pageMargin
        anchors.bottomMargin: 0

        // header
        Item {
            id: header
            width: parent.width
            height: 48
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                T { text: qsTr("Library"); font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
                T {
                    color: Theme.textMuted
                    text: root.model.count === backend.instanceCount ? Theme.count(backend.instanceCount, qsTr("1 instance"), qsTr("%1 instances"))
                          : qsTr("%1 of %2 instances").arg(root.model.count).arg(backend.instanceCount)
                }
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                WSearchField {
                    id: search
                    width: Math.min(320, header.width * 0.3)
                    placeholderText: qsTr("Search instances")
                    onTextChanged: root.model.filterText = text
                    Keys.onDownPressed: grid.forceActiveFocus()
                    Keys.onReturnPressed: grid.forceActiveFocus()
                }
                WButton { icon: "import"; text: qsTr("Import"); onClicked: backend.importInstance() }
                WButton { kind: "primary"; icon: "plus"; text: qsTr("New instance"); onClicked: backend.newInstance(root.model.groupFilter) }
            }
        }

        // filters
        Item {
            id: filters
            anchors.top: header.bottom
            anchors.topMargin: 22
            width: parent.width
            height: 36
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                Repeater {
                    model: root.loaders
                    delegate: Chip {
                        required property string modelData
                        text: modelData.length === 0 ? qsTr("All") : modelData === "Other" ? qsTr("Other") : modelData
                        dot: modelData.length === 0 || modelData === "Other" ? "transparent" : Theme.loaderColor(modelData)
                        checked: root.model.loaderFilter === modelData
                        onClicked: root.model.loaderFilter = modelData
                    }
                }
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                WComboBox {
                    id: groupBox
                    visible: backend.groups.length > 0
                    icon: "folder"
                    model: [qsTr("All groups")].concat(backend.groups)
                    onActivated: (index) => root.model.groupFilter = index === 0 ? "" : backend.groups[index - 1]
                }
                IconButton {
                    visible: root.model.groupFilter.length > 0
                    anchors.verticalCenter: parent.verticalCenter
                    icon: "more"
                    tip: qsTr("Group options")
                    onClicked: backend.showGroupMenu(root.model.groupFilter)
                }
                WComboBox {
                    icon: "sort"
                    textRole: "label"
                    model: root.sortModes
                    currentIndex: Math.max(0, root.sortModes.findIndex(m => m.key === root.model.sortMode))
                    onActivated: (index) => root.model.sortMode = root.sortModes[index].key
                }
            }
        }

        GridView {
            id: grid
            anchors { top: filters.bottom; topMargin: 22; left: parent.left; right: parent.right; bottom: parent.bottom }
            anchors.leftMargin: -8
            anchors.rightMargin: -8
            clip: true
            model: root.model
            boundsBehavior: Flickable.StopAtBounds
            readonly property int columns: Math.max(1, Math.floor(width / 236))
            cellWidth: Math.floor(width / columns)
            cellHeight: 272
            bottomMargin: Theme.pageMargin
            // Keyboard, like Prism's instance grid: arrows move, Enter opens, Ctrl+Enter plays, Delete/Backspace
            // deletes (after confirmation), F2 renames, Ctrl+K stops, Ctrl+G changes group, Ctrl+D duplicates,
            // the menu key opens the instance menu.
            keyNavigationEnabled: true
            activeFocusOnTab: true
            currentIndex: -1
            onActiveFocusChanged: if (activeFocus && currentIndex < 0 && count > 0) currentIndex = 0
            readonly property string currentId: currentItem ? currentItem.instanceId : ""
            Keys.onPressed: (event) => {
                if (currentId.length === 0)
                    return
                const ctrl = event.modifiers & Qt.ControlModifier
                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
                    ctrl ? backend.launch(currentId) : backend.openInstance(currentId)
                else if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace)
                    backend.remove(currentId)
                else if (event.key === Qt.Key_F2)
                    backend.rename(currentId)
                else if (event.key === Qt.Key_Menu)
                    backend.showInstanceMenu(currentId)
                else if (ctrl && event.key === Qt.Key_K)
                    backend.stop(currentId)
                else if (ctrl && event.key === Qt.Key_G)
                    backend.changeGroup(currentId)
                else if (ctrl && event.key === Qt.Key_D)
                    backend.duplicate(currentId)
                else if (ctrl && event.key === Qt.Key_I)
                    backend.openInstance(currentId)
                else
                    return
                event.accepted = true
            }
            delegate: Item {
                required property string instanceId
                required property string name
                required property string icon
                required property string loader
                required property string minecraft
                required property string lastPlayedText
                required property bool running
                required property int index
                width: grid.cellWidth
                height: grid.cellHeight
                InstanceCard {
                    keyboardFocus: grid.activeFocus && grid.currentIndex === parent.index
                    anchors.fill: parent
                    anchors.margins: 8
                    anchors.bottomMargin: 12
                    instanceId: parent.instanceId
                    name: parent.name
                    icon: parent.icon
                    loader: parent.loader
                    minecraft: parent.minecraft
                    lastPlayedText: parent.lastPlayedText
                    running: parent.running
                }
            }
            add: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.medium } }
            displaced: Transition { NumberAnimation { properties: "x,y"; duration: Theme.medium; easing.type: Easing.OutCubic } }
        }

        EmptyState {
            anchors.centerIn: grid
            visible: root.model.count === 0
            icon: backend.instanceCount === 0 ? "library" : "search"
            title: backend.instanceCount === 0 ? qsTr("No instances yet") : qsTr("Nothing matches")
            text: backend.instanceCount === 0 ? qsTr("Create one from scratch or install a modpack.")
                  : qsTr("Try another search or clear the filters.")
            WButton {
                visible: backend.instanceCount > 0
                text: qsTr("Clear filters")
                onClicked: { search.text = ""; root.model.loaderFilter = ""; root.model.groupFilter = ""; groupBox.currentIndex = 0 }
            }
            WButton { visible: backend.instanceCount === 0; kind: "primary"; icon: "plus"; text: qsTr("New instance"); onClicked: backend.newInstance() }
            WButton { visible: backend.instanceCount === 0; icon: "browse"; text: qsTr("Browse modpacks"); onClicked: backend.navigate("browse") }
        }
    }

    ParallelAnimation {
        id: enterAnim
        NumberAnimation { target: content; property: "opacity"; from: 0; to: 1; duration: Theme.medium; easing.type: Easing.OutCubic }
        NumberAnimation { target: content; property: "anchors.topMargin"; from: Theme.pageMargin + 12; to: Theme.pageMargin; duration: Theme.medium; easing.type: Easing.OutCubic }
    }
}
