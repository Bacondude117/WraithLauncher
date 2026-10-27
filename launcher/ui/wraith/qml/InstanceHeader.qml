import QtQuick
import "style"

// Instance page header: back link, cover banner with icon/name/badges/Play, and the tab bar.
Rectangle {
    id: root
    color: Theme.bg

    property string instanceId
    property var tabs: []
    property string currentTab: "content"
    signal tabSelected(string tab)

    readonly property var info: { backend.revision; return instanceId.length > 0 ? backend.instance(instanceId) : ({}) }
    readonly property bool running: info.running === true

    WButton {
        id: back
        x: Theme.pageMargin - 12
        y: 14
        kind: "ghost"
        compact: true
        icon: "back"
        text: qsTr("Library")
        onClicked: backend.navigate("library")
    }

    Rectangle {
        id: banner
        anchors { left: parent.left; right: parent.right; top: back.bottom; margins: Theme.pageMargin; topMargin: 8 }
        height: 176
        radius: Theme.radiusLarge
        color: Theme.surface
        border.width: 1
        border.color: Theme.border

        CoverArt {
            anchors.fill: parent
            anchors.margins: 1
            radius: Theme.radiusLarge - 1
            source: root.info.icon || ""
            seed: root.info.name || ""
            dim: 0.45
        }
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: Theme.radiusLarge - 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.withAlpha(Theme.bg, 0.9) }
                GradientStop { position: 0.6; color: Theme.withAlpha(Theme.bg, 0.5) }
                GradientStop { position: 1.0; color: Theme.withAlpha(Theme.bg, 0.15) }
            }
        }

        Rectangle {
            id: iconBox
            x: 24
            anchors.verticalCenter: parent.verticalCenter
            width: 112
            height: 112
            radius: 24
            color: Theme.withAlpha(Theme.surfaceRaised, 0.92)
            border.width: 1
            border.color: iconMouse.containsMouse ? Theme.accent : Theme.withAlpha("#ffffff", 0.08)
            Image {
                anchors.centerIn: parent
                width: 80
                height: 80
                source: root.info.icon || ""
                sourceSize: Qt.size(160, 160)
                mipmap: true
            }
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: Theme.withAlpha(Theme.bg, 0.55)
                opacity: iconMouse.containsMouse ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: Theme.fast } }
                Icon { anchors.centerIn: parent; name: "edit"; size: 24 }
            }
            MouseArea {
                id: iconMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: backend.changeIcon(root.instanceId)
            }
        }

        Column {
            anchors { left: iconBox.right; leftMargin: 24; right: actions.left; rightMargin: 24; verticalCenter: parent.verticalCenter }
            spacing: 10
            Row {
                spacing: 10
                width: parent.width
                T {
                    id: nameText
                    text: root.info.name || ""
                    font.pixelSize: 28
                    font.weight: Font.Bold
                    width: Math.min(implicitWidth, parent.width - 40)
                    MouseArea {
                        id: nameMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.rename(root.instanceId)
                    }
                }
                Icon {
                    anchors.verticalCenter: nameText.verticalCenter
                    name: "edit"
                    size: 16
                    color: Theme.textMuted
                    opacity: nameMouse.containsMouse ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.fast } }
                }
            }
            Row {
                spacing: 8
                Badge { text: root.info.loader || ""; tint: Theme.loaderColor(root.info.loader || "") }
                Badge { visible: (root.info.minecraft || "").length > 0; text: root.info.minecraft || ""; tint: Theme.textMuted }
                Badge { visible: (root.info.group || "").length > 0; text: root.info.group || ""; tint: Theme.violet }
                Badge { visible: root.info.managed === true; text: root.info.managedType || ""; tint: Theme.accent }
            }
            Row {
                spacing: 18
                Row {
                    spacing: 6
                    Icon { name: "clock"; size: 14; color: Theme.textMuted; anchors.verticalCenter: parent.verticalCenter }
                    T { text: root.info.playTimeText || ""; color: Theme.textMuted; font.pixelSize: Theme.fontCaption }
                }
                Row {
                    spacing: 6
                    Icon { name: "play"; size: 12; color: Theme.textMuted; anchors.verticalCenter: parent.verticalCenter }
                    T { text: root.info.lastPlayedText || ""; color: Theme.textMuted; font.pixelSize: Theme.fontCaption }
                }
                Row {
                    visible: root.running
                    spacing: 6
                    Rectangle { width: 8; height: 8; radius: 4; color: Theme.success; anchors.verticalCenter: parent.verticalCenter }
                    T { text: qsTr("Playing now"); color: Theme.success; font.pixelSize: Theme.fontCaption; font.weight: Font.DemiBold }
                }
            }
        }

        Row {
            id: actions
            anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
            spacing: 10
            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                size: 44
                filled: true
                icon: "folder"
                iconColor: Theme.text
                tip: qsTr("Open folder")
                onClicked: backend.openFolder(root.instanceId)
            }
            IconButton {
                anchors.verticalCenter: parent.verticalCenter
                size: 44
                filled: true
                icon: "more"
                iconColor: Theme.text
                tip: qsTr("More actions")
                onClicked: backend.showInstanceMenu(root.instanceId)
            }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                WButton {
                    implicitHeight: 48
                    implicitWidth: 140
                    radius: 12
                    kind: root.running ? "danger" : "primary"
                    icon: root.running ? "stop" : "play"
                    text: root.running ? qsTr("Stop") : qsTr("Play")
                    enabled: root.running || root.info.canLaunch === true
                    onClicked: root.running ? backend.stop(root.instanceId) : backend.launch(root.instanceId)
                }
                WButton {
                    implicitHeight: 48
                    implicitWidth: 36
                    radius: 12
                    kind: root.running ? "danger" : "primary"
                    icon: "chevron-down"
                    onClicked: backend.showLaunchMenu(root.instanceId)
                }
            }
        }
    }

    // tabs
    Item {
        id: tabBar
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; leftMargin: Theme.pageMargin; rightMargin: Theme.pageMargin }
        height: 50

        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }

        Row {
            id: tabRow
            anchors.bottom: parent.bottom
            spacing: 6
            Repeater {
                id: tabRepeater
                model: root.tabs
                delegate: Item {
                    id: tab
                    required property var modelData
                    readonly property bool active: root.currentTab === modelData.key
                    width: label.implicitWidth + 28
                    height: tabBar.height
                    T {
                        id: label
                        anchors.centerIn: parent
                        text: tab.modelData.label
                        font.pixelSize: 14
                        font.weight: tab.active ? Font.DemiBold : Font.Medium
                        color: tab.active ? Theme.text : tabMouse.containsMouse ? Theme.text : Theme.textMuted
                    }
                    MouseArea {
                        id: tabMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.tabSelected(tab.modelData.key)
                    }
                    onActiveChanged: if (active) indicator.target = tab
                    Component.onCompleted: if (active) indicator.target = tab
                }
            }
        }

        Rectangle {
            id: indicator
            property Item target: null
            anchors.bottom: parent.bottom
            height: 3
            radius: 2
            color: Theme.accent
            x: target ? target.x + 10 : 0
            width: target ? target.width - 20 : 0
            Behavior on x { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
            Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        }
    }
}
