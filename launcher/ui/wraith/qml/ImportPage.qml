import QtQuick
import QtQuick.Controls.Basic
import "style"

// "Import from another launcher": instances found in Prism, PolyMC, CurseForge, Modrinth App, ATLauncher, GDLauncher and
// the official launcher. They are copied; the other launcher's files are never changed.
Rectangle {
    id: root
    color: Theme.bg
    property var chosen: ({})
    readonly property var chosenList: Object.keys(chosen).filter(k => chosen[k]).map(Number)

    function pageShown() { importModel.scan(); chosen = ({}) }
    // used by the UI test script
    function chooseAll(unused) {
        const m = {}
        importModel.instances.forEach((inst, i) => { if (inst.importable) m[i] = true })
        chosen = m
    }

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
                T { text: qsTr("Import from another launcher"); font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
                T {
                    color: Theme.textMuted
                    text: importModel.launchers.length === 0 ? qsTr("No other launchers found on this computer.")
                          : qsTr("Found: %1. Instances are copied; the other launcher's files are never changed.").arg(importModel.launchers.join(", "))
                }
            }
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                WButton { icon: "folder"; text: qsTr("Choose a folder..."); onClicked: importModel.addFolder() }
                WButton {
                    kind: "primary"
                    icon: "import"
                    text: root.chosenList.length > 0 ? qsTr("Import %1").arg(root.chosenList.length) : qsTr("Import")
                    onClicked: { importModel.importSelected(root.chosenList); root.chosen = ({}) }
                }
            }
        }

        ListView {
            id: list
            anchors { top: header.bottom; topMargin: 22; left: parent.left; right: parent.right; bottom: parent.bottom }
            clip: true
            spacing: 8
            bottomMargin: Theme.pageMargin
            boundsBehavior: Flickable.StopAtBounds
            model: importModel.instances
            ScrollBar.vertical: ScrollBar { contentItem: Rectangle { implicitWidth: 6; radius: 3; color: "#2c3244" } }
            delegate: Rectangle {
                id: row
                required property var modelData
                required property int index
                width: list.width
                height: 64
                radius: Theme.radiusCard
                readonly property bool picked: root.chosen[index] === true
                color: picked ? Theme.surfaceHover : rowMouse.containsMouse ? Theme.surfaceHover : Theme.surface
                border.width: 1
                border.color: picked ? Theme.withAlpha(Theme.accent, 0.7) : Theme.border
                opacity: modelData.importable ? 1 : 0.5
                Rectangle {
                    id: box
                    x: 16
                    anchors.verticalCenter: parent.verticalCenter
                    width: 20; height: 20; radius: 6
                    color: row.picked ? Theme.accent : "transparent"
                    border.width: row.picked ? 0 : 1
                    border.color: Theme.borderStrong
                    Icon { anchors.centerIn: parent; visible: row.picked; name: "check"; size: 14; color: Theme.accentText }
                }
                Column {
                    anchors { left: box.right; leftMargin: 16; right: badges.left; rightMargin: 12; verticalCenter: parent.verticalCenter }
                    spacing: 3
                    T { width: parent.width; text: row.modelData.name; font.pixelSize: 14; font.weight: Font.DemiBold }
                    T { width: parent.width; text: row.modelData.importable ? row.modelData.path : qsTr("Minecraft version unknown (newer Modrinth App versions keep it in their database)"); color: Theme.textFaint; font.pixelSize: Theme.fontCaption }
                }
                Row {
                    id: badges
                    anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
                    spacing: 6
                    Badge { text: row.modelData.launcher; tint: Theme.violet }
                    Badge { text: row.modelData.loader; tint: Theme.loaderColor(row.modelData.loader) }
                    Badge { visible: row.modelData.minecraft.length > 0; text: row.modelData.minecraft; tint: Theme.textMuted }
                }
                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: row.modelData.importable
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { const c = Object.assign({}, root.chosen); c[row.index] = !c[row.index]; root.chosen = c }
                }
            }
        }
        EmptyState {
            anchors.centerIn: list
            visible: importModel.instances.length === 0
            icon: "import"
            title: qsTr("Nothing to import")
            text: qsTr("No instances were found in the usual places. Point Wraith at another launcher's folder instead.")
            WButton { icon: "folder"; text: qsTr("Choose a folder..."); onClicked: importModel.addFolder() }
        }
    }
}
