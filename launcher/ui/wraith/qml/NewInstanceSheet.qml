import QtQuick
import "style"

// "New instance" sheet over the whole shell: name, Minecraft version, mod loader, group.
Item {
    id: root

    property bool snapshots: false
    property string loader: "Vanilla"
    readonly property var versionList: newInstance.versions.filter(v => snapshots ? true : v.type === "release")
    readonly property string minecraft: versionBox.currentIndex >= 0 && versionBox.currentIndex < versionList.length ? versionList[versionBox.currentIndex].version : ""
    readonly property string suggestedName: loader === "Vanilla" ? minecraft : loader + " " + minecraft

    function opened() {
        nameField.text = ""
        groupField.text = newInstance.group
        loader = "Vanilla"
        versionBox.currentIndex = 0
        newInstance.selectLoader(loader, minecraft)
        nameField.forceActiveFocus()
        showAnim.restart()
    }
    function pickLoader(name) {
        loader = name
        newInstance.selectLoader(loader, minecraft)
    }
    onMinecraftChanged: newInstance.selectLoader(loader, minecraft)

    Keys.onEscapePressed: newInstance.close()

    Rectangle {
        id: dim
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.55)
        MouseArea { anchors.fill: parent; onClicked: newInstance.close() }
    }

    Rectangle {
        id: sheet
        anchors.centerIn: parent
        width: Math.min(580, parent.width - 64)
        height: content.implicitHeight + 56
        radius: Theme.radiusLarge
        color: Theme.surfaceRaised
        border.width: 1
        border.color: Theme.borderStrong
        MouseArea { anchors.fill: parent }  // keep clicks inside the sheet

        Column {
            id: content
            x: 28
            y: 28
            width: parent.width - 56
            spacing: 18

            Item {
                width: parent.width
                height: 34
                T { anchors.verticalCenter: parent.verticalCenter; text: qsTr("New instance"); font.pixelSize: 21; font.weight: Font.DemiBold }
                IconButton { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; icon: "close"; onClicked: newInstance.close() }
            }

            Column {
                width: parent.width
                spacing: 8
                T { text: qsTr("Name"); color: Theme.textMuted; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                WTextField { id: nameField; width: parent.width; placeholderText: root.suggestedName }
            }

            Column {
                width: parent.width
                spacing: 8
                Item {
                    width: parent.width
                    height: 18
                    T { text: qsTr("Minecraft version"); color: Theme.textMuted; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                    Row {
                        anchors.right: parent.right
                        spacing: 8
                        WSwitch {
                            scale: 0.8
                            anchors.verticalCenter: parent.verticalCenter
                            checked: root.snapshots
                            onToggled: (on) => { root.snapshots = on; versionBox.currentIndex = 0 }
                        }
                        T { text: qsTr("Show snapshots"); color: Theme.textMuted; font.pixelSize: Theme.fontCaption; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
                WComboBox {
                    id: versionBox
                    width: parent.width
                    enabled: !newInstance.loading
                    textRole: "version"
                    model: root.versionList
                    displayText: newInstance.loading ? qsTr("Loading versions...") : currentText
                }
            }

            Column {
                width: parent.width
                spacing: 8
                T { text: qsTr("Mod loader"); color: Theme.textMuted; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                Row {
                    spacing: 8
                    Repeater {
                        model: ["Vanilla", "Fabric", "Quilt", "Forge", "NeoForge"]
                        delegate: Chip {
                            required property string modelData
                            text: modelData === "Vanilla" ? qsTr("None") : modelData
                            dot: modelData === "Vanilla" ? "transparent" : Theme.loaderColor(modelData)
                            checked: root.loader === modelData
                            onClicked: root.pickLoader(modelData)
                        }
                    }
                }
                T {
                    visible: root.loader !== "Vanilla"
                    text: newInstance.loaderVersion
                    color: newInstance.loaderAvailable ? Theme.textMuted : Theme.warning
                    font.pixelSize: Theme.fontCaption
                }
            }

            Column {
                width: parent.width
                spacing: 8
                T { text: qsTr("Group"); color: Theme.textMuted; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                WTextField { id: groupField; width: parent.width; placeholderText: qsTr("No group") }
            }

            Item {
                width: parent.width
                height: 42
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    WButton { kind: "ghost"; compact: true; icon: "import"; text: qsTr("Import file"); onClicked: newInstance.importFile() }
                    WButton { kind: "ghost"; compact: true; icon: "sliders"; text: qsTr("More options"); onClicked: newInstance.advanced(groupField.text) }
                }
                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10
                    WButton { text: qsTr("Cancel"); onClicked: newInstance.close() }
                    WButton {
                        kind: "primary"
                        icon: "plus"
                        text: qsTr("Create")
                        enabled: root.minecraft.length > 0 && (root.loader === "Vanilla" || newInstance.loaderAvailable)
                        onClicked: newInstance.create(nameField.text, root.minecraft, root.loader, groupField.text)
                    }
                }
            }
        }
    }

    ParallelAnimation {
        id: showAnim
        NumberAnimation { target: dim; property: "opacity"; from: 0; to: 1; duration: Theme.medium }
        NumberAnimation { target: sheet; property: "opacity"; from: 0; to: 1; duration: Theme.medium }
        NumberAnimation { target: sheet; property: "scale"; from: 0.96; to: 1; duration: Theme.medium; easing.type: Easing.OutCubic }
    }
}
