import QtQuick
import QtQuick.Effects
import "style"

// Cover art made from an instance or pack icon: a blurred, darkened enlargement over a colour wash,
// clipped to rounded corners (optionally only the top ones).
Item {
    id: root
    property url source
    property string seed: ""
    property real radius: Theme.radiusCard
    property bool roundBottom: true
    property real dim: 0.45

    Item {
        id: art
        anchors.fill: parent
        visible: false
        layer.enabled: true

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: Theme.hashColor(root.seed) }
                GradientStop { position: 1; color: Qt.darker(Theme.hashColor(root.seed + "x"), 1.6) }
            }
        }
        Image {
            id: image
            anchors.centerIn: parent
            width: parent.width * 1.3
            height: parent.height * 1.3
            source: root.source
            sourceSize: Qt.size(64, 64)
            fillMode: Image.PreserveAspectCrop
            visible: false
        }
        MultiEffect {
            anchors.fill: image
            source: image
            visible: image.status === Image.Ready
            blurEnabled: true
            blur: 1.0
            blurMax: 48
            saturation: 0.15
        }
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0.04, 0.05, 0.07, root.dim)
        }
        // fade into the card below
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.45; color: "transparent" }
                GradientStop { position: 1.0; color: Theme.withAlpha(Theme.surface, 0.85) }
            }
        }
    }

    Item {
        id: mask
        anchors.fill: parent
        visible: false
        layer.enabled: true
        clip: true
        Rectangle {
            width: parent.width
            height: parent.height + (root.roundBottom ? 0 : root.radius)
            radius: root.radius
        }
    }

    MultiEffect {
        anchors.fill: parent
        source: art
        maskEnabled: true
        maskSource: mask
        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0
    }
}
