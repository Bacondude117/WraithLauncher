import QtQuick
import QtQuick.Controls.Basic
import "style"

// Instance "Snapshots" tab: a ring of automatic and manual snapshots. Restore always shows what will change first,
// compare shows the differences between two snapshots, and either can become a changelog.
Rectangle {
    id: root
    color: Theme.bg

    property var snapshots: null
    property string instanceName
    property string selectedId: ""
    property string compareFrom: ""     // set while picking the second snapshot to compare with
    property string mode: ""            // "", "preview", "compare", "changelog"
    property var detail: ({})
    property string changelogText: ""

    function refresh() {
        if (snapshots)
            snapshots.refresh()
    }
    // used by the UI test script
    function takeSnapshot(label) { snapshots.take(String(label), snapshots.includeWorldsByDefault) }
    function selectRow(row) { if (list.count > Number(row)) { list.currentIndex = Number(row); root.select(list.currentItem.snapshotId) } }
    function previewRow(row) { selectRow(row); root.showPreview() }
    function compareRows(rows) {
        const parts = String(rows).split(",")
        list.currentIndex = Number(parts[0]); const a = list.currentItem.snapshotId
        list.currentIndex = Number(parts[1]); const b = list.currentItem.snapshotId
        root.selectedId = b
        root.detail = snapshots.compare(a, b)
        root.mode = "compare"
    }
    function restoreShown(unused) { if (mode === "preview") snapshots.restore(detail.id) }
    function changelogShown(unused) { root.showChangelog() }

    function select(id) {
        if (compareFrom.length > 0 && compareFrom !== id) {
            detail = snapshots.compare(compareFrom, id)
            compareFrom = ""
            selectedId = id
            mode = "compare"
            return
        }
        compareFrom = ""
        selectedId = id
        mode = ""
    }
    function showPreview() {
        detail = snapshots.restorePreview(selectedId)
        mode = "preview"
    }
    function showChangelog() {
        if (mode === "compare")
            changelogText = snapshots.changelog(detail.olderId, detail.newerId)
        else
            changelogText = snapshots.changelog(selectedId, "")
        mode = "changelog"
    }

    Connections {
        target: root.snapshots
        function onListChanged() {
            if (root.selectedId.length > 0) {
                let found = false
                for (let i = 0; i < list.count; i++)
                    if (list.itemAtIndex(i) && list.itemAtIndex(i).snapshotId === root.selectedId) found = true
                if (!found) { root.selectedId = ""; root.mode = "" }
            }
        }
    }

    Item {
        id: main
        anchors { left: parent.left; top: parent.top; bottom: parent.bottom; right: panel.visible ? panel.left : parent.right }
        anchors.margins: Theme.pageMargin
        anchors.topMargin: 20
        anchors.bottomMargin: 0

        // toolbar
        Item {
            id: toolbar
            width: parent.width
            height: 40
            Column {
                anchors.verticalCenter: parent.verticalCenter
                T {
                    color: Theme.textMuted
                    text: root.snapshots ? Theme.count(root.snapshots.count, qsTr("1 snapshot"), qsTr("%1 snapshots"))
                                           + (root.snapshots.storageText.length > 0 ? "  ·  " + root.snapshots.storageText : "") : ""
                }
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                WButton {
                    visible: root.compareFrom.length > 0
                    kind: "ghost"
                    icon: "close"
                    text: qsTr("Cancel compare")
                    onClicked: root.compareFrom = ""
                }
                WButton {
                    kind: "primary"
                    icon: "snapshot"
                    text: qsTr("Take snapshot")
                    onClicked: takePopup.open()
                }
            }
        }

        // automatic snapshot settings
        Rectangle {
            id: settingsCard
            anchors.top: toolbar.bottom
            anchors.topMargin: 14
            width: parent.width
            height: settingsColumn.implicitHeight + 28
            radius: Theme.radiusCard
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
            Column {
                id: settingsColumn
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 14 }
                spacing: 10
                Item {
                    width: parent.width
                    height: 36
                    Column {
                        anchors { left: parent.left; right: autoSwitch.left; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        T { text: qsTr("Automatic snapshots"); font.weight: Font.DemiBold }
                        T {
                            width: parent.width
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontCaption
                            text: qsTr("Before mods, a modpack, Minecraft or a loader change (at most one every 5 minutes). Applies to every instance.")
                        }
                    }
                    WSwitch {
                        id: autoSwitch
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                        checked: root.snapshots ? root.snapshots.autoEnabled : false
                        onToggled: (on) => root.snapshots.autoEnabled = on
                    }
                }
                Row {
                    spacing: 18
                    Row {
                        spacing: 8
                        T { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Keep"); color: Theme.textMuted }
                        WComboBox {
                            id: ringBox
                            readonly property var sizes: [5, 10, 20, 50, 0]
                            model: [qsTr("5 snapshots"), qsTr("10 snapshots"), qsTr("20 snapshots"), qsTr("50 snapshots"), qsTr("all snapshots")]
                            currentIndex: root.snapshots ? Math.max(0, sizes.indexOf(root.snapshots.ringSize)) : 1
                            onActivated: (index) => root.snapshots.ringSize = sizes[index]
                        }
                        T {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("plus pinned ones")
                            color: Theme.textFaint
                            font.pixelSize: Theme.fontCaption
                        }
                    }
                    Row {
                        spacing: 8
                        WSwitch {
                            anchors.verticalCenter: parent.verticalCenter
                            checked: root.snapshots ? root.snapshots.includeWorldsByDefault : false
                            onToggled: (on) => root.snapshots.includeWorldsByDefault = on
                        }
                        T { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Include worlds in automatic snapshots"); color: Theme.textMuted }
                    }
                }
            }
        }

        T {
            id: compareHint
            visible: root.compareFrom.length > 0
            anchors { top: settingsCard.bottom; topMargin: 12 }
            color: Theme.accent
            text: qsTr("Pick the snapshot to compare with.")
        }

        ListView {
            id: list
            anchors { top: compareHint.visible ? compareHint.bottom : settingsCard.bottom; topMargin: 14; left: parent.left; right: parent.right; bottom: parent.bottom }
            clip: true
            spacing: 8
            model: root.snapshots
            currentIndex: -1
            boundsBehavior: Flickable.StopAtBounds
            bottomMargin: Theme.pageMargin
            ScrollBar.vertical: ScrollBar { contentItem: Rectangle { implicitWidth: 6; radius: 3; color: "#2c3244" } }
            delegate: Rectangle {
                id: row
                required property int index
                required property string snapshotId
                required property string title
                required property string trigger
                required property string createdText
                required property string createdFull
                required property bool automatic
                required property bool pinned
                required property bool includesWorlds
                required property int fileCount
                required property string sizeText
                width: list.width
                height: 68
                radius: Theme.radiusCard
                readonly property bool current: root.selectedId === snapshotId
                color: current ? Theme.surfaceHover : rowMouse.containsMouse ? Theme.surfaceHover : Theme.surface
                border.width: 1
                border.color: current ? Theme.withAlpha(Theme.accent, 0.7)
                              : root.compareFrom === snapshotId ? Theme.violet
                              : rowMouse.containsMouse ? Theme.borderStrong : Theme.border
                Behavior on color { ColorAnimation { duration: Theme.fast } }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.select(row.snapshotId)
                }
                Rectangle {
                    id: glyph
                    x: 14
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40; height: 40; radius: 10
                    color: Theme.withAlpha(row.automatic ? Theme.violet : Theme.accent, 0.14)
                    Icon { anchors.centerIn: parent; name: row.automatic ? "restore" : "snapshot"; size: 20; color: row.automatic ? Theme.violet : Theme.accent }
                }
                Column {
                    anchors { left: glyph.right; leftMargin: 14; right: badges.left; rightMargin: 12; verticalCenter: parent.verticalCenter }
                    spacing: 4
                    T { width: parent.width; text: row.title; font.pixelSize: 14; font.weight: Font.DemiBold }
                    T {
                        width: parent.width
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontCaption
                        text: row.createdText + "  ·  " + row.createdFull + "  ·  " + qsTr("%1 files, %2").arg(row.fileCount).arg(row.sizeText)
                    }
                }
                Row {
                    id: badges
                    anchors { right: parent.right; rightMargin: 14; verticalCenter: parent.verticalCenter }
                    spacing: 6
                    Badge { visible: row.includesWorlds; text: qsTr("Worlds"); tint: Theme.success }
                    Badge { visible: row.automatic; text: qsTr("Automatic"); tint: Theme.violet }
                    Icon { visible: row.pinned; anchors.verticalCenter: parent.verticalCenter; name: "pin"; size: 16; color: Theme.warning }
                }
            }
        }

        EmptyState {
            anchors.centerIn: list
            visible: !root.snapshots || root.snapshots.count === 0
            icon: "snapshot"
            title: qsTr("No snapshots yet")
            text: qsTr("A snapshot saves this instance's mods, configs and versions so you can go back. Wraith takes one automatically before changes.")
            WButton { kind: "primary"; icon: "snapshot"; text: qsTr("Take snapshot"); onClicked: takePopup.open() }
        }
    }

    // details / preview / compare / changelog
    Rectangle {
        id: panel
        visible: root.selectedId.length > 0
        width: 440
        anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
        color: Theme.sidebar
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.border }

        Flickable {
            anchors.fill: parent
            contentHeight: panelColumn.implicitHeight + 56
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { contentItem: Rectangle { implicitWidth: 6; radius: 3; color: "#2c3244" } }

            Column {
                id: panelColumn
                x: 24
                y: 24
                width: parent.width - 48
                spacing: 12

                Item {
                    width: parent.width
                    height: 32
                    T {
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.mode === "preview" ? qsTr("Restore preview") : root.mode === "compare" ? qsTr("Compare")
                              : root.mode === "changelog" ? qsTr("Changelog") : qsTr("Snapshot")
                        font.pixelSize: Theme.fontSection
                        font.weight: Font.DemiBold
                    }
                    IconButton { anchors.right: parent.right; icon: "close"; tip: qsTr("Close"); onClicked: { root.selectedId = ""; root.mode = "" } }
                }

                // actions for the selected snapshot
                Column {
                    visible: root.mode === ""
                    width: parent.width
                    spacing: 8
                    WButton { width: parent.width; kind: "primary"; icon: "restore"; text: qsTr("Restore..."); onClicked: root.showPreview() }
                    WButton {
                        width: parent.width
                        icon: "diff"
                        text: qsTr("Compare with another snapshot...")
                        onClicked: { root.compareFrom = root.selectedId; root.mode = ""; root.selectedId = "" }
                    }
                    WButton { width: parent.width; icon: "list"; text: qsTr("Changelog since this snapshot"); onClicked: root.showChangelog() }
                    Row {
                        width: parent.width
                        spacing: 8
                        WButton { width: (parent.width - 8) / 2; icon: "folder"; text: qsTr("Export to folder"); onClicked: root.snapshots.exportFolder(root.selectedId) }
                        WButton { width: (parent.width - 8) / 2; icon: "archive"; text: qsTr("Export as zip"); onClicked: root.snapshots.exportZip(root.selectedId) }
                    }
                    Row {
                        width: parent.width
                        spacing: 8
                        WButton {
                            width: (parent.width - 16) / 3
                            icon: "pin"
                            compact: true
                            readonly property bool pinned: list.currentItem && list.currentItem.snapshotId === root.selectedId ? list.currentItem.pinned : false
                            text: qsTr("Pin / unpin")
                            onClicked: {
                                for (let i = 0; i < list.count; i++) {
                                    const item = list.itemAtIndex(i)
                                    if (item && item.snapshotId === root.selectedId) { root.snapshots.setPinned(root.selectedId, !item.pinned); break }
                                }
                            }
                        }
                        WButton { width: (parent.width - 16) / 3; icon: "edit"; compact: true; text: qsTr("Rename"); onClicked: root.snapshots.rename(root.selectedId) }
                        WButton { width: (parent.width - 16) / 3; icon: "trash"; compact: true; kind: "danger"; text: qsTr("Delete"); onClicked: root.snapshots.remove(root.selectedId) }
                    }
                    T {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        elide: Text.ElideNone
                        color: Theme.textFaint
                        font.pixelSize: Theme.fontCaption
                        text: qsTr("Pinned snapshots are never removed to make room for new ones. Restoring saves the current state first, so it can be undone. Playtime and instance settings are never rolled back.")
                    }
                }

                // restore preview / compare: the changes, grouped
                Column {
                    visible: root.mode === "preview" || root.mode === "compare"
                    width: parent.width
                    spacing: 10
                    T { width: parent.width; text: root.detail.title || ""; font.pixelSize: 15; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                    T {
                        width: parent.width
                        wrapMode: Text.WordWrap
                        elide: Text.ElideNone
                        color: Theme.textMuted
                        text: (root.detail.total || 0) === 0 ? (root.mode === "preview" ? qsTr("Nothing to restore: the instance already matches this snapshot.") : qsTr("These snapshots are identical."))
                              : root.mode === "preview"
                                ? qsTr("%1 file(s) brought back, %2 removed, %3 replaced.").arg(root.detail.added).arg(root.detail.removed).arg(root.detail.changed)
                                : qsTr("%1 added, %2 removed, %3 changed.").arg(root.detail.added).arg(root.detail.removed).arg(root.detail.changed)
                    }
                    T {
                        visible: root.mode === "preview" && root.detail.includesWorlds === false
                        width: parent.width
                        wrapMode: Text.WordWrap
                        elide: Text.ElideNone
                        color: Theme.textFaint
                        font.pixelSize: Theme.fontCaption
                        text: qsTr("Worlds aren't in this snapshot, so they are left as they are.")
                    }
                    Repeater {
                        model: root.detail.groups || []
                        delegate: Column {
                            required property var modelData
                            width: panelColumn.width
                            spacing: 4
                            T { text: modelData.area + "  (" + modelData.items.length + ")"; font.weight: Font.DemiBold; color: Theme.text }
                            Repeater {
                                model: modelData.items
                                delegate: Row {
                                    required property var modelData
                                    spacing: 8
                                    Badge {
                                        anchors.verticalCenter: parent.verticalCenter
                                        readonly property string k: modelData.kind
                                        text: root.mode === "preview"
                                              ? (k === "added" ? qsTr("back") : k === "removed" ? qsTr("removed") : k === "changed" ? qsTr("replaced")
                                                 : k === "updated" ? qsTr("version") : k)
                                              : k
                                        tint: k === "added" || k === "enabled" ? Theme.success : k === "removed" || k === "disabled" ? Theme.danger : Theme.warning
                                    }
                                    T {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: panelColumn.width - 90
                                        text: modelData.name + (modelData.detail && modelData.detail.length > 0 && modelData.detail !== modelData.name ? "  " : "")
                                        font.pixelSize: Theme.fontCaption
                                    }
                                }
                            }
                        }
                    }
                    Row {
                        visible: root.mode === "preview"
                        width: parent.width
                        spacing: 8
                        WButton { width: (parent.width - 8) / 2; text: qsTr("Cancel"); onClicked: root.mode = "" }
                        WButton {
                            width: (parent.width - 8) / 2
                            kind: "danger"
                            icon: "restore"
                            text: qsTr("Restore")
                            onClicked: { root.snapshots.restore(root.detail.id); root.mode = "" }
                        }
                    }
                    T {
                        visible: root.mode === "preview" && root.detail.running === true
                        width: parent.width
                        wrapMode: Text.WordWrap
                        elide: Text.ElideNone
                        color: Theme.warning
                        text: qsTr("The game is running. Close it before restoring.")
                    }
                    WButton { visible: root.mode === "compare"; width: parent.width; icon: "list"; text: qsTr("Changelog"); onClicked: root.showChangelog() }
                }

                // changelog
                Column {
                    visible: root.mode === "changelog"
                    width: parent.width
                    spacing: 10
                    Rectangle {
                        width: parent.width
                        height: Math.max(220, changelogEdit.contentHeight + 24)
                        radius: Theme.radiusButton
                        color: Theme.surface
                        border.width: 1
                        border.color: Theme.border
                        TextEdit {
                            id: changelogEdit
                            anchors.fill: parent
                            anchors.margins: 12
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextEdit.Wrap
                            text: root.changelogText
                            color: Theme.text
                            font.family: "Consolas"
                            font.pixelSize: 12
                        }
                    }
                    Row {
                        width: parent.width
                        spacing: 8
                        WButton { width: (parent.width - 8) / 2; icon: "copy"; text: qsTr("Copy"); onClicked: root.snapshots.copyText(root.changelogText) }
                        WButton {
                            width: (parent.width - 8) / 2
                            icon: "download"
                            text: qsTr("Save as Markdown")
                            onClicked: root.snapshots.saveText(root.changelogText, root.instanceName + " changelog.md")
                        }
                    }
                }
            }
        }
    }

    // take a snapshot
    Popup {
        id: takePopup
        anchors.centerIn: parent
        width: 420
        modal: true
        padding: 22
        background: Rectangle { radius: Theme.radiusLarge; color: Theme.surfaceRaised; border.width: 1; border.color: Theme.borderStrong }
        onOpened: { labelField.text = ""; worldsSwitch.checked = root.snapshots.includeWorldsByDefault; labelField.forceActiveFocus() }
        contentItem: Column {
            spacing: 14
            T { text: qsTr("Take snapshot"); font.pixelSize: Theme.fontSection; font.weight: Font.DemiBold }
            WTextField { id: labelField; width: 376; placeholderText: qsTr("Name (optional), e.g. Before trying shaders"); onAccepted: saveButton.clicked() }
            Row {
                spacing: 10
                WSwitch { id: worldsSwitch; anchors.verticalCenter: parent.verticalCenter; onToggled: (on) => checked = on }
                T { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Include worlds"); color: Theme.textMuted }
            }
            Row {
                anchors.right: parent.right
                spacing: 8
                WButton { text: qsTr("Cancel"); onClicked: takePopup.close() }
                WButton {
                    id: saveButton
                    kind: "primary"
                    icon: "snapshot"
                    text: qsTr("Save snapshot")
                    onClicked: { takePopup.close(); root.snapshots.take(labelField.text, worldsSwitch.checked) }
                }
            }
        }
    }
}
