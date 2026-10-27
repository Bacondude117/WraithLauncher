import QtQuick
import QtQuick.Controls.Basic
import "style"

// Instance "Creator" tab: tools for people who make modpacks. Health check (dependencies and conflicts), export
// with a checklist, changelogs and config diffs between snapshots, and test launches. Everything runs locally and
// only suggests.
Rectangle {
    id: root
    color: Theme.bg

    property var model: null
    property string instanceName
    property string instanceId
    property var check: null              // last health check result
    property string exportFormat: ""
    property var checklist: []
    property var choices: []
    property string changelogText: ""
    property var configFiles: []
    property var diffLines: []
    property string diffPath: ""

    function refresh() {
        if (!model)
            return
        choices = model.snapshotChoices()
        pinnedText.text = qsTr("%1 pinned mod(s).").arg(model.pinnedCount())
    }
    // used by the UI test script
    function runCheck(unused) { model.runChecks() }
    function prepare(format) { root.exportFormat = String(format); model.prepareExport(String(format)) }
    function makeChangelog(unused) { root.generateChangelog() }
    function compareConfigs(unused) { root.compareConfig() }
    function openDiff(row) { if (configFiles.length > Number(row)) root.showDiff(configFiles[Number(row)].path) }

    function chosenId(box) { return box.currentIndex < choices.length ? choices[box.currentIndex].id : "" }
    function generateChangelog() { changelogText = model.changelog(chosenId(fromBox), chosenId(toBox)) }
    function compareConfig() {
        configFiles = model.configChanges(chosenId(diffFromBox), chosenId(diffToBox))
        diffLines = []
        diffPath = ""
    }
    function showDiff(path) {
        diffPath = path
        diffLines = model.fileDiff(chosenId(diffFromBox), chosenId(diffToBox), path)
    }

    Connections {
        target: root.model
        function onChecksFinished(result) { root.check = result }
        function onChecklistReady(format, items) { root.exportFormat = format; root.checklist = items }
    }

    component Card: Rectangle {
        default property alias content: inner.data
        property string title
        property string subtitle
        property string icon
        width: parent ? parent.width : 400
        height: inner.implicitHeight + header.height + 40
        radius: Theme.radiusCard
        color: Theme.surface
        border.width: 1
        border.color: Theme.border
        Row {
            id: header
            x: 18; y: 16
            spacing: 12
            height: 40
            Rectangle {
                width: 36; height: 36; radius: 10
                color: Theme.withAlpha(Theme.accent, 0.12)
                Icon { anchors.centerIn: parent; name: parent.parent.parent.icon; size: 18; color: Theme.accent }
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                T { text: parent.parent.parent.title; font.pixelSize: 15; font.weight: Font.DemiBold }
                T { text: parent.parent.parent.subtitle; color: Theme.textMuted; font.pixelSize: Theme.fontCaption }
            }
        }
        Column {
            id: inner
            x: 18
            y: header.y + header.height + 12
            width: parent.width - 36
            spacing: 10
        }
    }

    component StateDot: Rectangle {
        property string level
        width: 10; height: 10; radius: 5
        color: level === "error" ? Theme.danger : level === "warn" || level === "warning" ? Theme.warning : level === "ok" ? Theme.success : Theme.textMuted
    }

    Flickable {
        anchors.fill: parent
        contentHeight: column.implicitHeight + 64
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { contentItem: Rectangle { implicitWidth: 6; radius: 3; color: "#2c3244" } }

        Column {
            id: column
            x: Theme.pageMargin
            y: 20
            width: Math.min(parent.width - 2 * Theme.pageMargin, 1100)
            spacing: 16

            // ---------------------------------------------------------------- health check
            Card {
                title: qsTr("Health check")
                subtitle: qsTr("Missing or disabled dependencies, incompatible mods, duplicates, wrong loader or Minecraft version")
                icon: "check-circle"
                Row {
                    spacing: 10
                    WButton { kind: "primary"; icon: "zap"; text: root.model && root.model.checking ? qsTr("Checking...") : qsTr("Check mods"); onClicked: root.model.runChecks() }
                    T {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: root.check !== null
                        color: Theme.textMuted
                        text: root.check ? qsTr("%1 mod files read%2.").arg(root.check.files)
                                           .arg(root.check.unknown > 0 ? qsTr(", %1 without readable metadata").arg(root.check.unknown) : "") : ""
                    }
                }
                T {
                    visible: root.check !== null && root.check.issues.length === 0
                    text: qsTr("No problems found.")
                    color: Theme.success
                }
                Repeater {
                    model: root.check ? root.check.issues : []
                    delegate: Rectangle {
                        required property var modelData
                        width: parent.width
                        height: issueColumn.implicitHeight + 20
                        radius: Theme.radiusButton
                        color: Theme.surfaceRaised
                        StateDot { x: 12; y: 15; level: modelData.severity }
                        Column {
                            id: issueColumn
                            x: 32; y: 10
                            width: parent.width - 44
                            spacing: 3
                            T { width: parent.width; text: modelData.title; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                            T { visible: modelData.detail.length > 0; width: parent.width; text: modelData.detail; color: Theme.textMuted; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                            T { visible: modelData.suggestion.length > 0; width: parent.width; text: "→ " + modelData.suggestion; color: Theme.accent; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                            T { width: parent.width; text: modelData.files.join(", "); color: Theme.textFaint; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                        }
                    }
                }
            }

            // ---------------------------------------------------------------- export
            Card {
                title: qsTr("Export")
                subtitle: qsTr("A checklist first, then Prism's export dialog (choose files, name and version there)")
                icon: "upload"
                Row {
                    spacing: 10
                    WButton { icon: "box"; text: qsTr("Modrinth (.mrpack)"); onClicked: root.prepare("mrpack") }
                    WButton { icon: "box"; text: qsTr("CurseForge (.zip)"); onClicked: root.prepare("curseforge") }
                }
                Repeater {
                    model: root.checklist
                    delegate: Row {
                        required property var modelData
                        width: parent.width
                        spacing: 10
                        StateDot { y: 5; level: modelData.state }
                        Column {
                            width: parent.width - 20
                            T { width: parent.width; text: modelData.title; font.weight: Font.Medium; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                            T { visible: modelData.detail.length > 0; width: parent.width; text: modelData.detail; color: Theme.textMuted; font.pixelSize: Theme.fontCaption; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                        }
                    }
                }
                Row {
                    visible: root.checklist.length > 0
                    spacing: 10
                    WButton {
                        kind: "primary"
                        icon: "upload"
                        text: root.exportFormat === "curseforge" ? qsTr("Continue to CurseForge export") : qsTr("Continue to Modrinth export")
                        onClicked: root.model.openExport(root.exportFormat)
                    }
                    WButton { icon: "snapshot"; text: qsTr("Snapshot this release..."); onClicked: releasePopup.open() }
                }
            }

            // ---------------------------------------------------------------- changelog
            Card {
                title: qsTr("Changelog")
                subtitle: qsTr("Mods added, updated and removed between two snapshots (or a snapshot and now)")
                icon: "list"
                Row {
                    spacing: 10
                    T { anchors.verticalCenter: parent.verticalCenter; text: qsTr("From"); color: Theme.textMuted }
                    WComboBox { id: fromBox; width: 300; textRole: "title"; model: root.choices; currentIndex: Math.min(1, root.choices.length - 1) }
                    T { anchors.verticalCenter: parent.verticalCenter; text: qsTr("to"); color: Theme.textMuted }
                    WComboBox { id: toBox; width: 300; textRole: "title"; model: root.choices.concat([{ id: "", title: qsTr("Now") }]); currentIndex: 0 }
                    WButton { kind: "primary"; text: qsTr("Generate"); onClicked: root.generateChangelog() }
                }
                T { visible: root.choices.length === 0; text: qsTr("Take snapshots (Snapshots tab, or \"Snapshot this release\" above) to compare releases."); color: Theme.textMuted }
                Rectangle {
                    visible: root.changelogText.length > 0
                    width: parent.width
                    height: changelogEdit.contentHeight + 24
                    radius: Theme.radiusButton
                    color: Theme.surfaceRaised
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
                    visible: root.changelogText.length > 0
                    spacing: 10
                    WButton { icon: "copy"; text: qsTr("Copy"); onClicked: { backend.toast(qsTr("Copied to the clipboard"), "success"); changelogEdit.selectAll(); changelogEdit.copy(); changelogEdit.deselect() } }
                }
            }

            // ---------------------------------------------------------------- config diff
            Card {
                title: qsTr("Config diff")
                subtitle: qsTr("Which config and option files changed, line by line")
                icon: "diff"
                Row {
                    spacing: 10
                    T { anchors.verticalCenter: parent.verticalCenter; text: qsTr("From"); color: Theme.textMuted }
                    WComboBox { id: diffFromBox; width: 300; textRole: "title"; model: root.choices; currentIndex: 0 }
                    T { anchors.verticalCenter: parent.verticalCenter; text: qsTr("to"); color: Theme.textMuted }
                    WComboBox { id: diffToBox; width: 300; textRole: "title"; model: root.choices.concat([{ id: "", title: qsTr("Now") }]); currentIndex: root.choices.length }
                    WButton { kind: "primary"; text: qsTr("Compare"); onClicked: root.compareConfig() }
                }
                Flow {
                    width: parent.width
                    spacing: 8
                    Repeater {
                        model: root.configFiles
                        delegate: Chip {
                            required property var modelData
                            text: modelData.path.split("/").slice(-2).join("/")
                            dot: modelData.kind === "added" ? Theme.success : modelData.kind === "removed" ? Theme.danger : Theme.warning
                            checked: root.diffPath === modelData.path
                            onClicked: root.showDiff(modelData.path)
                        }
                    }
                }
                Rectangle {
                    visible: root.diffLines.length > 0
                    width: parent.width
                    height: Math.min(420, diffColumn.implicitHeight + 20)
                    radius: Theme.radiusButton
                    color: Theme.surfaceRaised
                    clip: true
                    Flickable {
                        anchors.fill: parent
                        anchors.margins: 10
                        contentHeight: diffColumn.implicitHeight
                        boundsBehavior: Flickable.StopAtBounds
                        Column {
                            id: diffColumn
                            width: parent.width
                            Repeater {
                                model: root.diffLines
                                delegate: Rectangle {
                                    required property var modelData
                                    width: diffColumn.width
                                    height: 18
                                    color: modelData.kind === "added" ? Theme.withAlpha(Theme.success, 0.12) : modelData.kind === "removed" ? Theme.withAlpha(Theme.danger, 0.12) : "transparent"
                                    T {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: parent.width
                                        text: modelData.text
                                        font.family: "Consolas"
                                        font.pixelSize: 12
                                        color: modelData.kind === "hunk" ? Theme.violet : modelData.kind === "added" ? Theme.success : modelData.kind === "removed" ? Theme.danger : Theme.textMuted
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ---------------------------------------------------------------- test launch and pins
            Card {
                title: qsTr("Test launch")
                subtitle: qsTr("Snapshot, play, and see what the session changed; roll back from Snapshots if you want")
                icon: "rocket"
                Row {
                    spacing: 10
                    WButton { kind: "primary"; icon: "play"; text: qsTr("Test launch"); onClicked: root.model.testLaunch() }
                    T { anchors.verticalCenter: parent.verticalCenter; color: Theme.textMuted; text: qsTr("Nothing is rolled back unless you choose to.") }
                }
            }
            Card {
                title: qsTr("Version pins")
                subtitle: qsTr("Pinned mods are skipped by update checks. Pin them in Content (pin icon on a mod).")
                icon: "pin"
                T { id: pinnedText; color: Theme.textMuted }
            }
        }
    }

    Popup {
        id: releasePopup
        anchors.centerIn: parent
        width: 400
        modal: true
        padding: 22
        background: Rectangle { radius: Theme.radiusLarge; color: Theme.surfaceRaised; border.width: 1; border.color: Theme.borderStrong }
        onOpened: { releaseField.text = ""; releaseField.forceActiveFocus() }
        contentItem: Column {
            spacing: 14
            T { text: qsTr("Snapshot this release"); font.pixelSize: Theme.fontSection; font.weight: Font.DemiBold }
            WTextField { id: releaseField; width: 356; placeholderText: qsTr("Version, e.g. 1.2.0"); onAccepted: okButton.clicked() }
            Row {
                anchors.right: parent.right
                spacing: 8
                WButton { text: qsTr("Cancel"); onClicked: releasePopup.close() }
                WButton { id: okButton; kind: "primary"; text: qsTr("Save"); onClicked: { releasePopup.close(); root.model.snapshotForRelease(releaseField.text); root.refresh() } }
            }
        }
    }
}
