import QtQuick
import "style"

// The slim left navigation: Home, Library, Browse, Servers ... Settings, More and the account chip.
Rectangle {
    id: root
    color: Theme.sidebar

    function navigate(page) { backend.navigate(page) }

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }

    Row {
        id: brand
        x: 24
        y: 22
        spacing: 11
        Rectangle {
            width: 34
            height: 34
            radius: 11
            gradient: Gradient {
                GradientStop { position: 0; color: Theme.withAlpha(Theme.accent, 0.28) }
                GradientStop { position: 1; color: Theme.withAlpha(Theme.violet, 0.22) }
            }
            Icon { anchors.centerIn: parent; name: "ghost"; size: 20; color: Theme.accent }
        }
        Column {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0
            T { text: "Wraith"; font.pixelSize: 17; font.weight: Font.Bold }
            T { text: qsTr("Launcher"); font.pixelSize: 11; color: Theme.textFaint }
        }
    }

    Column {
        anchors { left: parent.left; right: parent.right; top: brand.bottom; topMargin: 28 }
        spacing: 4
        NavItem { icon: "home"; text: qsTr("Home"); active: backend.currentPage === "home"; onClicked: root.navigate("home") }
        NavItem {
            icon: "library"; text: qsTr("Library")
            active: backend.currentPage === "library" || backend.currentPage === "instance"
            badge: backend.runningCount > 0 ? backend.runningCount + " " + qsTr("playing") : ""
            onClicked: root.navigate("library")
        }
        NavItem { icon: "browse"; text: qsTr("Browse"); active: backend.currentPage === "browse"; onClicked: root.navigate("browse") }
        NavItem { icon: "servers"; text: qsTr("Servers"); active: backend.currentPage === "servers"; onClicked: root.navigate("servers") }
    }

    Column {
        anchors { left: parent.left; right: parent.right; bottom: account.top; bottomMargin: 10 }
        spacing: 4
        NavItem { icon: "settings"; text: qsTr("Settings"); active: backend.currentPage === "settings"; onClicked: root.navigate("settings") }
        NavItem { icon: "more"; text: qsTr("More"); onClicked: backend.showMoreMenu() }
    }

    Rectangle {
        id: account
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 12 }
        height: 56
        radius: Theme.radiusCard
        color: accountMouse.containsMouse ? Theme.surfaceHover : Theme.surface
        border.width: 1
        border.color: Theme.border
        Behavior on color { ColorAnimation { duration: Theme.fast } }

        Rectangle {
            id: avatar
            x: 10
            anchors.verticalCenter: parent.verticalCenter
            width: 36
            height: 36
            radius: 10
            color: backend.accountName.length > 0 ? Theme.withAlpha(Theme.violet, 0.25) : Theme.surfaceRaised
            T {
                anchors.centerIn: parent
                visible: backend.accountName.length > 0
                text: backend.accountName.charAt(0).toUpperCase()
                color: Theme.violet
                font.pixelSize: 16
                font.weight: Font.Bold
            }
            Icon { anchors.centerIn: parent; visible: backend.accountName.length === 0; name: "user"; size: 18; color: Theme.textMuted }
        }
        Column {
            anchors { left: avatar.right; leftMargin: 10; right: chevron.left; rightMargin: 6; verticalCenter: parent.verticalCenter }
            T {
                width: parent.width
                text: backend.accountName.length > 0 ? backend.accountName : qsTr("Add an account")
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }
            T { width: parent.width; text: backend.accountType; font.pixelSize: 11; color: Theme.textFaint }
        }
        Icon { id: chevron; anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter } name: "chevron-down"; size: 14; color: Theme.textFaint }
        MouseArea {
            id: accountMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: backend.showAccountMenu()
        }
    }
}
