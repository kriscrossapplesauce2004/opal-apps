import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import Opal.Material
import "Fixtures.js" as F
// Photos editor: tools as a vertical rail, adjustments as expressive sliders, a before/after split on the photo.
Rectangle {
    color: "#0b0b0a"
    readonly property var cur: F.photos.filter(p => p.n.indexOf("dr5-keyart") === 0)[0] || F.photos[0]
    RowLayout {
        anchors.fill: parent; spacing: 0
        ColumnLayout { Layout.preferredWidth: 92; Layout.fillHeight: true; Layout.topMargin: 18; spacing: 8
            Repeater { model: [{i: "auto_fix_high", t: "Auto"}, {i: "crop_rotate", t: "Crop"}, {i: "tune", t: "Adjust", on: true}, {i: "filter_vintage", t: "Filters"}, {i: "draw", t: "Markup"}]
                ColumnLayout { required property var modelData; Layout.alignment: Qt.AlignHCenter; spacing: 4
                    Rectangle { Layout.alignment: Qt.AlignHCenter; implicitWidth: 56; implicitHeight: 32; radius: 16; color: modelData.on ? Theme.secondaryContainer : "transparent"
                        Glyph { anchors.centerIn: parent; name: modelData.i; fill: modelData.on ? 1 : 0; color: modelData.on ? Theme.onSecondaryContainer : "#e6e1d6" } }
                    MText { Layout.alignment: Qt.AlignHCenter; text: modelData.t; role: "label"; color: "#e6e1d6" } } }
            Item { Layout.fillHeight: true } }
        Item { Layout.fillWidth: true; Layout.fillHeight: true
            Item { id: stage; anchors.centerIn: parent; width: Math.min(parent.width - 80, (parent.height - 140) * cur.w / cur.h); height: width * cur.h / cur.w
                RoundedImage { id: before; anchors.fill: parent; radius: 14; source: cur.u; sourceSize: Qt.size(1400, 800) }
                // After: the right part shows the adjusted photo.
                Item { x: parent.width * .5; width: parent.width * .5; height: parent.height; clip: true
                    Image { id: after; x: -parent.x; width: stage.width; height: stage.height; source: cur.u; sourceSize: Qt.size(1400, 800); fillMode: Image.PreserveAspectCrop; visible: false }
                    MultiEffect { x: -parent.x; width: stage.width; height: stage.height; source: after; brightness: .08; contrast: .25; saturation: .35 } }
                Rectangle { x: parent.width * .5 - 1; width: 2; height: parent.height; color: "#ffffff" }
                Rectangle { x: parent.width * .5 - 20; y: parent.height / 2 - 20; width: 40; height: 40; radius: 20; color: "#ffffff"; Glyph { anchors.centerIn: parent; name: "compare_arrows"; color: "#111111" } }
                Rectangle { x: 12; y: 12; implicitWidth: 74; implicitHeight: 28; radius: 14; color: Qt.rgba(0, 0, 0, .55); MText { anchors.centerIn: parent; text: "Before"; role: "labelS"; color: "#ffffff" } }
                Rectangle { anchors.right: parent.right; anchors.rightMargin: 12; y: 12; implicitWidth: 64; implicitHeight: 28; radius: 14; color: Qt.rgba(0, 0, 0, .55); MText { anchors.centerIn: parent; text: "After"; role: "labelS"; color: "#ffffff" } } }
            RowLayout { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 16; spacing: 8
                MButton { text: "Cancel"; contained: true }
                MButton { text: "Save copy"; tonal: true }
                MButton { text: "Save"; filled: true } } }
        Rectangle { Layout.preferredWidth: 320; Layout.fillHeight: true; color: Theme.surfaceLow
            ColumnLayout { anchors.fill: parent; anchors.margins: 20; spacing: 6
                MText { text: "Adjust"; role: "title" }
                Repeater { model: [{t: "Brightness", i: "brightness_6", v: 58}, {t: "Contrast", i: "contrast", v: 66}, {t: "Saturation", i: "palette", v: 70}, {t: "Warmth", i: "thermostat", v: 52}, {t: "Highlights", i: "highlight", v: 44}, {t: "Shadows", i: "shadow", v: 56}, {t: "Vignette", i: "vignette", v: 30}]
                    ColumnLayout { required property var modelData; Layout.fillWidth: true; spacing: 0
                        RowLayout { Layout.fillWidth: true; MText { text: modelData.t; role: "label"; Layout.fillWidth: true } MText { text: (modelData.v - 50 > 0 ? "+" : "") + (modelData.v - 50); role: "labelS"; color: Theme.subtext } }
                        ExpressiveSlider { Layout.fillWidth: true; icon: modelData.i; value: modelData.v; implicitHeight: 48; trackHeight: 30 } } }
                Item { Layout.fillHeight: true }
                MButton { text: "Reset all"; icon: "restart_alt"; contained: true; Layout.fillWidth: true } } }
    }
}
