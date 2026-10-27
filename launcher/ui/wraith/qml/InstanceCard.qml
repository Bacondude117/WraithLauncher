import QtQuick
import "style"

// Library/Home card for one instance: cover art, icon, name, loader + version, last played, hover Play.
Item {
    id: root
    property string instanceId
    property string name
    property url icon
    property string loader
    property string minecraft
    property string lastPlayedText
    property string playTimeText
    property bool running: false

    implicitWidth: 216
    implicitHeight: 252
    readonly property bool hovered: mouse.containsMouse || play.hovered || more.hovered

    Rectangle {
        id: card
        anchors.fill: parent
        anchors.topMargin: root.hovered ? 0 : 3
        anchors.bottomMargin: root.hovered ? 3 : 0
        Behavior on anchors.topMargin { NumberAnimation { duration: Theme.medium; easing.type: Easing.OutCubic } }
        Behavior on anchors.bottomMargin { NumberAnimation { duration: Theme.medium; easing.type: Easing.OutCubic } }
        radius: Theme.radiusCard
        color: root.hovered ? Theme.surfaceHover : Theme.surface
        border.width: 1
        border.color: root.running ? Theme.withAlpha(Theme.success, 0.6) : root.hovered ? Theme.borderStrong : Theme.border
        Behavior on color { ColorAnimation { duration: Theme.fast } }
        Behavior on border.color { ColorAnimation { duration: Theme.fast } }

        CoverArt {
            id: cover
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 1 }
            height: 118
            source: root.icon
            seed: root.name
            radius: Theme.radiusCard - 1
            roundBottom: false
        }

        Rectangle {
            visible: root.running
            anchors { left: parent.left; top: parent.top; margins: 10 }
            height: 22
            width: playingRow.implicitWidth + 16
            radius: 11
            color: Theme.withAlpha("#06120b", 0.8)
            Row {
                id: playingRow
                anchors.centerIn: parent
                spacing: 6
                Rectangle { width: 7; height: 7; radius: 4; color: Theme.success; anchors.verticalCenter: parent.verticalCenter }
                T { text: qsTr("Playing"); color: Theme.success; font.pixelSize: 12; font.weight: Font.DemiBold }
            }
        }

        Rectangle {
            id: iconFrame
            x: 14
            y: cover.height - 30
            width: 60
            height: 60
            radius: 14
            color: Theme.surfaceRaised
            border.width: 2
            border.color: card.color
            Image {
                anchors.centerIn: parent
                width: 44
                height: 44
                source: root.icon
                sourceSize: Qt.size(88, 88)
                smooth: true
                mipmap: true
            }
        }

        Column {
            anchors { left: parent.left; right: parent.right; top: iconFrame.bottom; margins: 14; topMargin: 10 }
            spacing: 8
            T {
                width: parent.width
                text: root.name
                font.pixelSize: Theme.fontCardTitle
                font.weight: Font.DemiBold
            }
            Row {
                spacing: 6
                Badge { text: root.loader; tint: Theme.loaderColor(root.loader) }
                Badge { visible: root.minecraft.length > 0; text: root.minecraft; tint: Theme.textMuted }
            }
            T {
                width: parent.width
                text: root.lastPlayedText
                color: Theme.textFaint
                font.pixelSize: Theme.fontCaption
            }
        }

        PlayButton {
            id: play
            property bool hovered: hoverArea.containsMouse
            anchors { right: parent.right; rightMargin: 14; top: cover.bottom; topMargin: -22 }
            running: root.running
            opacity: root.hovered || root.running ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Theme.fast } }
            onClicked: root.running ? backend.stop(root.instanceId) : backend.launch(root.instanceId)
            MouseArea { id: hoverArea; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
        }

        IconButton {
            id: more
            property bool hovered: moreHover.containsMouse
            anchors { right: parent.right; top: parent.top; margins: 8 }
            size: 30
            icon: "more"
            iconColor: Theme.text
            filled: true
            opacity: root.hovered ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Theme.fast } }
            onClicked: backend.showInstanceMenu(root.instanceId)
            MouseArea { id: moreHover; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        z: -1
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: (event) => {
            if (event.button === Qt.RightButton)
                backend.showInstanceMenu(root.instanceId)
            else
                backend.openInstance(root.instanceId)
        }
    }
}
