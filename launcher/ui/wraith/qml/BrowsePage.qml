import QtQuick
import QtQuick.Controls.Basic
import "style"

// Browse: one search across Modrinth, CurseForge, FTB, ATLauncher and Technic, filters on the left,
// results as cards, details and install on the right.
Rectangle {
    id: root
    color: Theme.bg

    readonly property var model: browseModel
    property bool searched: false
    property int selected: -1
    property var detail: ({})
    property var versions: []
    property string versionsState: ""  // "", "loading", "ready", "failed"

    function pageShown() {
        if (!searched)
            search(searchField.text)
        searchField.forceActiveFocus()
        enterAnim.restart()
    }
    function search(query) {
        searched = true
        if (searchField.text !== query)
            searchField.text = query
        selected = -1
        model.search(query)
    }
    function select(row) {
        row = Number(row)
        selected = row
        detail = model.details(row)
        versions = []
        versionsState = ""
        if (detail.hits && detail.hits.some(h => h.source === "Modrinth")) {
            versionsState = "loading"
            model.loadVersions(row)
        }
    }

    Connections {
        target: root.model
        function onVersionsLoaded(row, list) {
            if (row !== root.selected) return
            root.versions = list
            root.versionsState = "ready"
            versionBox.currentIndex = 0
        }
        function onVersionsFailed(row, error) {
            if (row === root.selected) root.versionsState = "failed"
        }
        function onFiltersChanged() { root.selected = -1 }
    }

    Timer { id: debounce; interval: 400; onTriggered: root.search(searchField.text) }

    // filter panel
    Rectangle {
        id: filters
        width: 250
        anchors { top: parent.top; bottom: parent.bottom; left: parent.left }
        color: Theme.bg
        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.border }

        Column {
            anchors { fill: parent; margins: 24; topMargin: 32 }
            spacing: 10

            T { text: qsTr("Sources"); font.pixelSize: Theme.fontSection; font.weight: Font.DemiBold }
            Item { width: 1; height: 2 }
            Repeater {
                model: root.model.sources
                delegate: Rectangle {
                    id: sourceRow
                    required property var modelData
                    width: parent.width
                    height: 40
                    radius: Theme.radiusButton
                    color: rowMouse.containsMouse && modelData.available ? Theme.surface : "transparent"
                    opacity: modelData.available ? 1 : 0.45
                    Rectangle {
                        id: box
                        x: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: 18
                        height: 18
                        radius: 5
                        color: sourceRow.modelData.enabled && sourceRow.modelData.available ? Theme.accent : "transparent"
                        border.width: sourceRow.modelData.enabled && sourceRow.modelData.available ? 0 : 1
                        border.color: Theme.borderStrong
                        Icon { anchors.centerIn: parent; visible: sourceRow.modelData.enabled && sourceRow.modelData.available; name: "check"; size: 13; color: Theme.accentText }
                    }
                    Rectangle {
                        id: dot
                        anchors { left: box.right; leftMargin: 12; verticalCenter: parent.verticalCenter }
                        width: 8; height: 8; radius: 4
                        color: Theme.sourceColor(sourceRow.modelData.name)
                    }
                    T {
                        anchors { left: dot.right; leftMargin: 8; verticalCenter: parent.verticalCenter }
                        text: sourceRow.modelData.name
                        font.weight: Font.Medium
                    }
                    T {
                        anchors { right: parent.right; rightMargin: 10; verticalCenter: parent.verticalCenter }
                        visible: !sourceRow.modelData.pending
                        text: !sourceRow.modelData.available ? qsTr("off") : sourceRow.modelData.failed ? qsTr("error") : String(sourceRow.modelData.count)
                        color: sourceRow.modelData.failed ? Theme.danger : Theme.textFaint
                        font.pixelSize: Theme.fontCaption
                    }
                    BusyIndicator {
                        anchors { right: parent.right; rightMargin: 4; verticalCenter: parent.verticalCenter }
                        visible: sourceRow.modelData.pending
                        running: visible
                        width: 22; height: 22
                    }
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: sourceRow.modelData.available
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.model.setSourceEnabled(sourceRow.modelData.name, !sourceRow.modelData.enabled)
                    }
                }
            }
            T {
                visible: root.model.curseForgeMissing
                width: parent.width
                wrapMode: Text.WordWrap
                elide: Text.ElideNone
                color: Theme.textFaint
                font.pixelSize: Theme.fontCaption
                text: qsTr("CurseForge needs an API key (Settings › Network › APIs).")
            }

            Item { width: 1; height: 14 }
            T { text: qsTr("Sort by"); font.pixelSize: Theme.fontSection; font.weight: Font.DemiBold }
            Row {
                spacing: 8
                Chip { text: qsTr("Relevance"); checked: root.model.sortMode === "relevance"; onClicked: root.model.sortMode = "relevance" }
                Chip { text: qsTr("Downloads"); checked: root.model.sortMode === "downloads"; onClicked: root.model.sortMode = "downloads" }
            }
        }

        WButton {
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 20 }
            kind: "ghost"
            compact: true
            icon: "sliders"
            text: qsTr("Per-source browsers")
            onClicked: root.model.openAdvanced()
        }
    }

    // results
    Item {
        id: main
        anchors { left: filters.right; right: details.visible ? details.left : parent.right; top: parent.top; bottom: parent.bottom }
        anchors.margins: Theme.pageMargin
        anchors.bottomMargin: 0

        Item {
            id: header
            width: parent.width
            height: 48
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                T { text: qsTr("Browse modpacks"); font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
                T {
                    color: Theme.textMuted
                    text: root.model.status.length > 0 ? root.model.status
                          : Theme.count(root.model.count, qsTr("1 modpack"), qsTr("%1 modpacks"))
                }
            }
        }

        WSearchField {
            id: searchField
            anchors.top: header.bottom
            anchors.topMargin: 20
            width: parent.width
            implicitHeight: 46
            font.pixelSize: 14
            placeholderText: qsTr("Search every modpack source at once")
            onTextEdited: debounce.restart()
            onAccepted: { debounce.stop(); root.search(text) }
        }

        ListView {
            id: list
            anchors { top: searchField.bottom; topMargin: 18; left: parent.left; right: parent.right; bottom: parent.bottom }
            clip: true
            spacing: 10
            model: root.model
            boundsBehavior: Flickable.StopAtBounds
            bottomMargin: Theme.pageMargin
            ScrollBar.vertical: ScrollBar { contentItem: Rectangle { implicitWidth: 6; radius: 3; color: "#2c3244" } }
            delegate: Rectangle {
                id: card
                required property int index
                required property string name
                required property string author
                required property string summary
                required property string icon
                required property string downloads
                required property var sources
                width: list.width
                height: 104
                radius: Theme.radiusCard
                readonly property bool current: root.selected === index
                color: current ? Theme.surfaceHover : cardMouse.containsMouse ? Theme.surfaceHover : Theme.surface
                border.width: 1
                border.color: current ? Theme.withAlpha(Theme.accent, 0.7) : cardMouse.containsMouse ? Theme.borderStrong : Theme.border
                Behavior on color { ColorAnimation { duration: Theme.fast } }

                Rectangle {
                    id: iconBox
                    x: 16
                    anchors.verticalCenter: parent.verticalCenter
                    width: 72; height: 72; radius: 14
                    color: Theme.surfaceRaised
                    clip: true
                    Image {
                        anchors.fill: parent
                        source: card.icon
                        sourceSize: Qt.size(144, 144)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }
                }
                Column {
                    anchors { left: iconBox.right; leftMargin: 16; right: meta.left; rightMargin: 16; verticalCenter: parent.verticalCenter }
                    spacing: 5
                    Row {
                        spacing: 8
                        T { text: card.name; font.pixelSize: 15; font.weight: Font.DemiBold }
                        T { visible: card.author.length > 0; text: qsTr("by %1").arg(card.author); color: Theme.textMuted; font.pixelSize: Theme.fontCaption; anchors.baseline: parent.children[0].baseline }
                    }
                    T {
                        width: parent.width
                        text: card.summary
                        color: Theme.textMuted
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2
                    }
                    Row {
                        spacing: 6
                        Repeater {
                            model: card.sources
                            delegate: Badge { required property string modelData; text: modelData; tint: Theme.sourceColor(modelData) }
                        }
                    }
                }
                Column {
                    id: meta
                    anchors { right: parent.right; rightMargin: 20; verticalCenter: parent.verticalCenter }
                    spacing: 4
                    Row {
                        anchors.right: parent.right
                        spacing: 6
                        visible: card.downloads.length > 0
                        Icon { name: "download"; size: 14; color: Theme.textMuted; anchors.verticalCenter: parent.verticalCenter }
                        T { text: card.downloads; color: Theme.textMuted; font.pixelSize: Theme.fontCaption }
                    }
                }
                MouseArea {
                    id: cardMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.select(card.index)
                }
            }
        }

        EmptyState {
            anchors.centerIn: list
            visible: root.searched && !root.model.running && root.model.count === 0
            icon: "search"
            title: qsTr("No modpacks found")
            text: qsTr("Try fewer words, or turn more sources on.")
        }
        BusyIndicator {
            anchors.centerIn: list
            visible: root.model.running && root.model.count === 0
            running: visible
        }
    }

    // details
    Rectangle {
        id: details
        visible: root.selected >= 0
        width: 400
        anchors { top: parent.top; bottom: parent.bottom; right: parent.right }
        color: Theme.sidebar
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.border }

        Flickable {
            anchors.fill: parent
            contentHeight: detailColumn.implicitHeight + 64
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: detailColumn
                x: 28
                y: 28
                width: parent.width - 56
                spacing: 14

                Item {
                    width: parent.width
                    height: 32
                    IconButton { anchors.right: parent.right; icon: "close"; tip: qsTr("Close"); onClicked: root.selected = -1 }
                }
                Rectangle {
                    width: 112; height: 112; radius: 24
                    color: Theme.surfaceRaised
                    clip: true
                    Image { anchors.fill: parent; source: root.detail.icon || ""; sourceSize: Qt.size(224, 224); fillMode: Image.PreserveAspectCrop }
                }
                T { width: parent.width; text: root.detail.name || ""; font.pixelSize: 22; font.weight: Font.Bold; wrapMode: Text.WordWrap; elide: Text.ElideNone }
                T {
                    width: parent.width
                    visible: (root.detail.author || "").length > 0
                    text: qsTr("by %1").arg(root.detail.author || "") + ((root.detail.downloads || "").length > 0 ? "  ·  " + root.detail.downloads : "")
                    color: Theme.textMuted
                }
                T { width: parent.width; text: root.detail.summary || ""; color: Theme.text; wrapMode: Text.WordWrap; elide: Text.ElideNone }

                Item { width: 1; height: 6 }
                T { text: qsTr("Install"); font.pixelSize: Theme.fontSection; font.weight: Font.DemiBold }

                // Modrinth: pick a version and install directly
                Column {
                    width: parent.width
                    spacing: 10
                    visible: root.versionsState.length > 0
                    T { text: qsTr("From Modrinth"); color: Theme.textMuted; font.pixelSize: Theme.fontCaption }
                    BusyIndicator { visible: root.versionsState === "loading"; running: visible; width: 28; height: 28 }
                    T { visible: root.versionsState === "failed"; text: qsTr("Couldn't load versions."); color: Theme.danger }
                    WComboBox {
                        id: versionBox
                        visible: root.versionsState === "ready"
                        width: parent.width
                        textRole: "label"
                        model: root.versions.map(v => ({ label: v.number + "  ·  " + v.loaders + " " + v.minecraft }))
                    }
                    WButton {
                        visible: root.versionsState === "ready" && root.versions.length > 0
                        width: parent.width
                        kind: "primary"
                        icon: "download"
                        text: qsTr("Install")
                        onClicked: root.model.installModrinth(root.selected, root.versions[versionBox.currentIndex].id)
                    }
                }

                // other sources: their own page, with the search filled in
                Repeater {
                    model: (root.detail.hits || []).filter(h => h.source !== "Modrinth")
                    delegate: WButton {
                        required property var modelData
                        width: detailColumn.width
                        icon: "external"
                        text: qsTr("Install with %1...").arg(modelData.source)
                        onClicked: root.model.installWith(root.selected, modelData.source)
                    }
                }

                Item { width: 1; height: 6 }
                T { text: qsTr("Available on"); font.pixelSize: Theme.fontSection; font.weight: Font.DemiBold }
                Flow {
                    width: parent.width
                    spacing: 8
                    Repeater {
                        model: root.detail.hits || []
                        delegate: Chip {
                            required property var modelData
                            text: modelData.source
                            dot: Theme.sourceColor(modelData.source)
                            onClicked: if (modelData.url.length > 0) backend.openUrl(modelData.url)
                        }
                    }
                }
            }
        }
    }

    ParallelAnimation {
        id: enterAnim
        NumberAnimation { target: main; property: "opacity"; from: 0; to: 1; duration: Theme.medium; easing.type: Easing.OutCubic }
    }
}
