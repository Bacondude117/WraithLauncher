import QtQuick
import QtQuick.Controls.Basic
import "style"

// Servers: every friend server (an instance turned into a local dedicated server), with start/stop and manage.
Rectangle {
    id: root
    color: Theme.bg

    property var servers: []
    function refresh() { servers = backend.friendServers() }
    function pageShown() { refresh(); page.enter() }
    function search(q) {}
    function openPicker(unused) { picker.open() }

    Connections {
        target: backend
        function onServersChanged() { root.refresh() }
        function onInstancesUpdated() { root.refresh() }
    }

    PageFlick {
        id: page
        anchors.fill: parent

        Item {
            width: parent.width
            height: 56
            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                T { text: qsTr("Friend servers"); font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
                T { color: Theme.textMuted; text: qsTr("Host any instance as a server your friends can join, with client-only mods left out.") }
            }
            WButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                kind: "primary"
                icon: "plus"
                text: qsTr("New server")
                onClicked: picker.open()
            }
        }

        Column {
            width: parent.width
            spacing: 12
            Repeater {
                model: root.servers
                delegate: Rectangle {
                    id: card
                    required property var modelData
                    width: parent.width
                    height: 92
                    radius: Theme.radiusCard
                    color: cardMouse.containsMouse ? Theme.surfaceHover : Theme.surface
                    border.width: 1
                    border.color: modelData.online ? Theme.withAlpha(Theme.success, 0.5) : cardMouse.containsMouse ? Theme.borderStrong : Theme.border
                    Behavior on color { ColorAnimation { duration: Theme.fast } }
                    MouseArea { id: cardMouse; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }

                    Rectangle {
                        id: iconBox
                        x: 16
                        anchors.verticalCenter: parent.verticalCenter
                        width: 60; height: 60; radius: 14
                        color: Theme.surfaceRaised
                        Image { anchors.centerIn: parent; width: 42; height: 42; source: card.modelData.icon; sourceSize: Qt.size(84, 84); mipmap: true }
                    }
                    Column {
                        anchors { left: iconBox.right; leftMargin: 16; verticalCenter: parent.verticalCenter }
                        spacing: 7
                        T { text: card.modelData.name; font.pixelSize: 15; font.weight: Font.DemiBold }
                        Row {
                            spacing: 8
                            Badge { text: card.modelData.loader; tint: Theme.loaderColor(card.modelData.loader) }
                            Badge { visible: card.modelData.minecraft.length > 0; text: card.modelData.minecraft; tint: Theme.textMuted }
                            Badge { text: qsTr("Port %1").arg(card.modelData.port); tint: Theme.textMuted }
                        }
                    }
                    Row {
                        anchors { right: parent.right; rightMargin: 18; verticalCenter: parent.verticalCenter }
                        spacing: 12
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            height: 28
                            width: statusRow.implicitWidth + 22
                            radius: 14
                            readonly property color tint: card.modelData.online ? Theme.success : card.modelData.busy ? Theme.warning : Theme.textFaint
                            color: Theme.withAlpha(tint, 0.14)
                            Row {
                                id: statusRow
                                anchors.centerIn: parent
                                spacing: 7
                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: parent.parent.tint
                                    anchors.verticalCenter: parent.verticalCenter
                                    SequentialAnimation on opacity {
                                        running: card.modelData.busy
                                        loops: Animation.Infinite
                                        NumberAnimation { to: 0.3; duration: 600 }
                                        NumberAnimation { to: 1; duration: 600 }
                                    }
                                }
                                T { text: card.modelData.state; color: parent.parent.tint; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                            }
                        }
                        WButton {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Manage")
                            onClicked: backend.openInstance(card.modelData.id, "friend")
                        }
                        WButton {
                            anchors.verticalCenter: parent.verticalCenter
                            implicitWidth: 110
                            kind: card.modelData.serverRunning ? "danger" : "primary"
                            icon: card.modelData.serverRunning ? "stop" : "play"
                            text: card.modelData.serverRunning ? qsTr("Stop") : qsTr("Start")
                            enabled: card.modelData.ready && !(card.modelData.busy && !card.modelData.serverRunning)
                            onClicked: card.modelData.serverRunning ? backend.stopServer(card.modelData.id) : backend.startServer(card.modelData.id)
                        }
                    }
                }
            }
        }

        Item {
            visible: root.servers.length === 0
            width: parent.width
            height: 360
            EmptyState {
                anchors.centerIn: parent
                icon: "servers"
                title: qsTr("No friend servers yet")
                text: qsTr("Pick an instance and Wraith builds a server from it: the right server software for its loader, its mods minus the client-only ones, and its configs.")
                WButton { kind: "primary"; icon: "plus"; text: qsTr("New server"); onClicked: picker.open() }
            }
        }
    }

    // instance picker for "New server"
    Popup {
        id: picker
        anchors.centerIn: parent
        width: 460
        height: Math.min(520, root.height - 80)
        modal: true
        padding: 0
        Overlay.modal: Rectangle { color: Theme.withAlpha("#000000", 0.55) }
        enter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.medium }
                NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: Theme.medium; easing.type: Easing.OutCubic }
            }
        }
        background: Rectangle { radius: Theme.radiusLarge; color: Theme.surfaceRaised; border.width: 1; border.color: Theme.borderStrong }

        Column {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 12
            T { text: qsTr("New friend server"); font.pixelSize: 19; font.weight: Font.DemiBold }
            T { width: parent.width; text: qsTr("Which instance should your friends play?"); color: Theme.textMuted }
            ListView {
                width: parent.width
                height: picker.height - 130
                clip: true
                spacing: 4
                model: backend.instances
                boundsBehavior: Flickable.StopAtBounds
                delegate: Rectangle {
                    required property string instanceId
                    required property string name
                    required property string icon
                    required property string loader
                    required property string minecraft
                    width: ListView.view.width
                    height: 54
                    radius: 10
                    color: rowMouse.containsMouse ? Theme.surfaceHover : "transparent"
                    Image { id: rowIcon; x: 10; anchors.verticalCenter: parent.verticalCenter; width: 34; height: 34; source: icon; sourceSize: Qt.size(68, 68) }
                    T { anchors { left: rowIcon.right; leftMargin: 12; verticalCenter: parent.verticalCenter } text: name; font.weight: Font.Medium }
                    Row {
                        anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        spacing: 6
                        Badge { text: loader; tint: Theme.loaderColor(loader) }
                        Badge { visible: minecraft.length > 0; text: minecraft; tint: Theme.textMuted }
                    }
                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { picker.close(); backend.newServer(instanceId) }
                    }
                }
            }
        }
    }
}
