import QtQuick
import QtQuick.Controls.Basic
import "style"

// Instance "Logs" tab: every log and crash report, filtered by level, searchable, live for latest.log, with a plain
// language crash analysis on top. Prism's live console and log tools are under "Live console".
Rectangle {
    id: root
    color: Theme.bg

    property var model: null
    property string instanceName
    property string instanceId
    signal advanced()

    function refresh() { if (model) model.refresh() }
    // used by the UI test script
    function openFile(path) { model.open(String(path)) }
    function filter(level) { model.levelFilter = String(level) }

    Item {
        anchors.fill: parent
        anchors.margins: Theme.pageMargin
        anchors.topMargin: 20
        anchors.bottomMargin: 0

        // toolbar
        Row {
            id: toolbar
            width: parent.width
            spacing: 10
            WComboBox {
                id: fileBox
                width: 300
                textRole: "name"
                model: root.model ? root.model.files : []
                currentIndex: {
                    if (!root.model) return -1
                    for (let i = 0; i < root.model.files.length; i++)
                        if (root.model.files[i].path === root.model.currentFile) return i
                    return -1
                }
                displayText: currentIndex >= 0 ? currentText : qsTr("No logs yet")
                onActivated: (index) => root.model.open(root.model.files[index].path)
            }
            Chip { anchors.verticalCenter: parent.verticalCenter; text: qsTr("All"); checked: root.model && root.model.levelFilter === "all"; onClicked: root.model.levelFilter = "all" }
            Chip {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Warnings")
                count: root.model ? String(root.model.warningCount) : ""
                dot: Theme.warning
                checked: root.model && root.model.levelFilter === "warn"
                onClicked: root.model.levelFilter = "warn"
            }
            Chip {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Errors")
                count: root.model ? String(root.model.errorCount) : ""
                dot: Theme.danger
                checked: root.model && root.model.levelFilter === "error"
                onClicked: root.model.levelFilter = "error"
            }
            WSearchField {
                width: Math.max(160, toolbar.width - x - actions.width - 10)
                placeholderText: qsTr("Search the log")
                onTextChanged: root.model.search = text
            }
        }
        Row {
            id: actions
            anchors.right: parent.right
            anchors.top: toolbar.top
            spacing: 6
            IconButton {
                icon: "alert"
                tip: qsTr("Next error")
                onClicked: {
                    const row = root.model.nextError(list.currentIndex)
                    if (row >= 0) { list.currentIndex = row; list.positionViewAtIndex(row, ListView.Center) }
                }
            }
            IconButton { icon: "copy"; tip: qsTr("Copy what is shown"); onClicked: root.model.copyVisible() }
            IconButton { icon: "folder"; tip: qsTr("Open folder"); onClicked: root.model.openFolder() }
            WButton { icon: "terminal"; compact: true; text: qsTr("Live console"); onClicked: root.advanced() }
        }

        // crash analysis
        Column {
            id: findingsColumn
            anchors { top: toolbar.bottom; topMargin: 14; left: parent.left; right: parent.right }
            spacing: 8
            visible: root.model && root.model.findings.length > 0
            Repeater {
                model: root.model ? root.model.findings : []
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    width: findingsColumn.width
                    height: findingColumn.implicitHeight + 24
                    radius: Theme.radiusCard
                    color: Theme.withAlpha(Theme.danger, index === 0 ? 0.10 : 0.05)
                    border.width: 1
                    border.color: Theme.withAlpha(Theme.danger, index === 0 ? 0.45 : 0.2)
                    Icon { x: 16; y: 14; name: "bug"; size: 20; color: Theme.danger }
                    Column {
                        id: findingColumn
                        x: 48; y: 12
                        width: parent.width - 64
                        spacing: 4
                        T { width: parent.width; text: modelData.title; font.pixelSize: 15; font.weight: Font.DemiBold; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                        T { width: parent.width; text: modelData.explanation; color: Theme.textMuted; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                        T { width: parent.width; text: "→ " + modelData.suggestion; color: Theme.accent; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                        T {
                            visible: modelData.evidence.length > 0
                            width: parent.width
                            text: modelData.evidence
                            color: Theme.textFaint
                            font.family: "Consolas"
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                        Row {
                            visible: index === 0
                            spacing: 8
                            topPadding: 4
                            WButton { visible: modelData.rollback && root.model.hasSnapshots; compact: true; icon: "restore"; text: qsTr("Roll back to a snapshot..."); onClicked: root.model.showSnapshots() }
                            WButton { visible: modelData.memory; compact: true; icon: "cpu"; text: qsTr("Memory settings"); onClicked: root.model.showSettings() }
                            WButton { visible: modelData.java; compact: true; icon: "cpu"; text: qsTr("Java settings"); onClicked: root.model.showSettings() }
                        }
                    }
                }
            }
        }

        // the log
        Rectangle {
            anchors { top: findingsColumn.visible ? findingsColumn.bottom : toolbar.bottom; topMargin: 14; left: parent.left; right: parent.right; bottom: parent.bottom; bottomMargin: Theme.pageMargin }
            radius: Theme.radiusCard
            color: Theme.surface
            border.width: 1
            border.color: Theme.border
            clip: true

            ListView {
                id: list
                anchors.fill: parent
                anchors.margins: 10
                model: root.model
                currentIndex: -1
                boundsBehavior: Flickable.StopAtBounds
                reuseItems: true
                ScrollBar.vertical: ScrollBar { contentItem: Rectangle { implicitWidth: 8; radius: 4; color: "#2c3244" } }
                delegate: Rectangle {
                    required property int index
                    required property string text
                    required property string level
                    required property int lineNumber
                    required property bool matches
                    width: list.width - 12
                    height: lineText.implicitHeight + 2
                    color: ListView.isCurrentItem ? Theme.withAlpha(Theme.accent, 0.15) : matches ? Theme.withAlpha(Theme.warning, 0.12) : "transparent"
                    T {
                        id: number
                        width: 54
                        horizontalAlignment: Text.AlignRight
                        text: lineNumber
                        color: Theme.textFaint
                        font.family: "Consolas"
                        font.pixelSize: 11
                    }
                    Text {
                        id: lineText
                        anchors { left: number.right; leftMargin: 12; right: parent.right }
                        text: parent.text
                        wrapMode: Text.WrapAnywhere
                        textFormat: Text.PlainText
                        font.family: "Consolas"
                        font.pixelSize: 12
                        color: parent.level === "error" || parent.level === "fatal" ? Theme.danger
                               : parent.level === "warn" ? Theme.warning
                               : parent.level === "debug" ? Theme.textFaint : Theme.text
                    }
                    MouseArea { anchors.fill: parent; onClicked: list.currentIndex = parent.index }
                }
            }
            EmptyState {
                anchors.centerIn: parent
                visible: !root.model || root.model.totalCount === 0 || list.count === 0
                icon: "terminal"
                title: root.model && root.model.totalCount > 0 ? qsTr("Nothing matches") : qsTr("No log yet")
                text: root.model && root.model.totalCount > 0 ? qsTr("Try another filter or search.") : qsTr("Logs appear here after the game has run.")
            }
        }
    }
}
