import QtQuick
import QtQuick.Layouts
import "style"

// Home: continue playing, recent instances, quick actions.
Rectangle {
    id: root
    color: Theme.bg

    property var recent: { backend.revision; return backend.recentInstances(7) }
    readonly property var hero: recent.length > 0 ? recent[0] : null

    function pageShown() { page.enter() }

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

    PageFlick {
        id: page
        anchors.fill: parent

        // greeting
        Item {
            width: parent.width
            height: 64
            Column {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                T { text: qsTr("Welcome back"); font.pixelSize: Theme.fontTitle; font.weight: Font.DemiBold }
                T {
                    color: Theme.textMuted
                    text: backend.instanceCount === 0 ? qsTr("Let's set up your first game.")
                          : Theme.count(backend.instanceCount, qsTr("1 instance"), qsTr("%1 instances")) + "  ·  " + backend.totalPlayTime + " " + qsTr("played")
                }
            }
            WButton {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                kind: "primary"
                icon: "plus"
                text: qsTr("New instance")
                onClicked: backend.newInstance()
            }
        }

        // empty library
        Item {
            visible: backend.instanceCount === 0
            width: parent.width
            height: 340
            EmptyState {
                anchors.centerIn: parent
                title: qsTr("Your library is empty")
                text: qsTr("Create an instance with any Minecraft version and loader, or install a modpack from ATLauncher, CurseForge, FTB, Modrinth or Technic.")
                WButton { kind: "primary"; icon: "plus"; text: qsTr("New instance"); onClicked: backend.newInstance() }
                WButton { icon: "browse"; text: qsTr("Browse modpacks"); onClicked: backend.navigate("browse") }
            }
        }

        // continue playing
        Rectangle {
            id: heroCard
            visible: root.hero !== null
            width: parent.width
            height: 236
            radius: Theme.radiusLarge
            color: Theme.surface
            border.width: 1
            border.color: heroMouse.containsMouse ? Theme.borderStrong : Theme.border

            CoverArt {
                anchors.fill: parent
                anchors.margins: 1
                radius: Theme.radiusLarge - 1
                source: root.hero ? root.hero.icon : ""
                seed: root.hero ? root.hero.name : ""
                dim: 0.4
            }
            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                radius: Theme.radiusLarge - 1
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Theme.withAlpha(Theme.bg, 0.92) }
                    GradientStop { position: 0.55; color: Theme.withAlpha(Theme.bg, 0.55) }
                    GradientStop { position: 1.0; color: Theme.withAlpha(Theme.bg, 0.05) }
                }
            }
            MouseArea {
                id: heroMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (event) => {
                    if (!root.hero) return
                    if (event.button === Qt.RightButton) backend.showInstanceMenu(root.hero.id)
                    else backend.openInstance(root.hero.id)
                }
            }

            Row {
                anchors { left: parent.left; leftMargin: 32; verticalCenter: parent.verticalCenter }
                spacing: 24
                Rectangle {
                    width: 112
                    height: 112
                    radius: 24
                    color: Theme.withAlpha(Theme.surfaceRaised, 0.9)
                    border.width: 1
                    border.color: Theme.withAlpha("#ffffff", 0.08)
                    Image {
                        anchors.centerIn: parent
                        width: 80
                        height: 80
                        source: root.hero ? root.hero.icon : ""
                        sourceSize: Qt.size(160, 160)
                        mipmap: true
                    }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 10
                    T { text: qsTr("CONTINUE PLAYING"); color: Theme.accent; font.pixelSize: 12; font.weight: Font.Bold; font.letterSpacing: 1.2 }
                    T { text: root.hero ? root.hero.name : ""; font.pixelSize: 28; font.weight: Font.Bold; width: Math.min(implicitWidth, heroCard.width - 420) }
                    Row {
                        spacing: 8
                        Badge { text: root.hero ? root.hero.loader : ""; tint: Theme.loaderColor(root.hero ? root.hero.loader : "") }
                        Badge { visible: root.hero && root.hero.minecraft.length > 0; text: root.hero ? root.hero.minecraft : ""; tint: Theme.textMuted }
                        T {
                            anchors.verticalCenter: parent.verticalCenter
                            leftPadding: 4
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontCaption
                            text: root.hero ? root.hero.lastPlayedText + "  ·  " + root.hero.playTimeText : ""
                        }
                    }
                }
            }
            Row {
                anchors { right: parent.right; rightMargin: 32; verticalCenter: parent.verticalCenter }
                spacing: 10
                WButton {
                    anchors.verticalCenter: parent.verticalCenter
                    icon: "more"
                    onClicked: if (root.hero) backend.showInstanceMenu(root.hero.id)
                }
                WButton {
                    anchors.verticalCenter: parent.verticalCenter
                    kind: root.hero && root.hero.running ? "danger" : "primary"
                    implicitHeight: 48
                    implicitWidth: 150
                    icon: root.hero && root.hero.running ? "stop" : "play"
                    text: root.hero && root.hero.running ? qsTr("Stop") : qsTr("Play")
                    onClicked: {
                        if (!root.hero) return
                        root.hero.running ? backend.stop(root.hero.id) : backend.launch(root.hero.id)
                    }
                }
            }
        }

        // recent
        Column {
            visible: root.recent.length > 1
            width: parent.width
            spacing: 14
            Item {
                width: parent.width
                height: 28
                T { anchors.verticalCenter: parent.verticalCenter; text: qsTr("Jump back in"); font.pixelSize: Theme.fontSection; font.weight: Font.DemiBold }
                WButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    kind: "ghost"
                    compact: true
                    text: qsTr("View library")
                    icon: "chevron-right"
                    onClicked: backend.navigate("library")
                }
            }
            Row {
                id: recentRow
                width: parent.width
                spacing: 16
                readonly property int columns: Math.max(1, Math.floor((width + spacing) / (216 + spacing)))
                Repeater {
                    model: root.recent.slice(1, 1 + recentRow.columns)
                    delegate: InstanceCard {
                        width: 216
                        height: 252
                        instanceId: modelData.id
                        name: modelData.name
                        icon: modelData.icon
                        loader: modelData.loader
                        minecraft: modelData.minecraft
                        lastPlayedText: modelData.lastPlayedText
                        running: modelData.running
                    }
                }
            }
        }

        // quick actions
        Column {
            width: parent.width
            spacing: 14
            T { text: qsTr("Quick actions"); font.pixelSize: Theme.fontSection; font.weight: Font.DemiBold }
            GridLayout {
                width: parent.width
                columns: width > 1000 ? 4 : 2
                columnSpacing: 16
                rowSpacing: 16
                QuickAction {
                    Layout.fillWidth: true
                    icon: "plus"; title: qsTr("New instance"); subtitle: qsTr("Any version, any loader")
                    onClicked: backend.newInstance()
                }
                QuickAction {
                    Layout.fillWidth: true
                    icon: "browse"; tint: Theme.violet; title: qsTr("Browse modpacks"); subtitle: qsTr("Five sources, one search")
                    onClicked: backend.navigate("browse")
                }
                QuickAction {
                    Layout.fillWidth: true
                    icon: "import"; tint: Theme.warning; title: qsTr("Import"); subtitle: qsTr("Zip, .mrpack or CurseForge pack")
                    onClicked: backend.importInstance()
                }
                QuickAction {
                    Layout.fillWidth: true
                    icon: "servers"; tint: Theme.success; title: qsTr("Host a friend server"); subtitle: qsTr("Turn an instance into a server")
                    onClicked: backend.navigate("servers")
                }
            }
        }
    }
}
