import QtQuick
import QtQuick.Layouts
import Opal.Material
// Item count, the selection chip and the zoom slider.
RowLayout {
    id: strip
    property var model
    property real zoom: .35
    signal zoomMoved(real value)
    spacing: 8
    MText { text: strip.model ? qsTr("%n item(s)", "", strip.model.count) : ""; role: "label"; color: Theme.subtext }
    Rectangle { visible: strip.model && strip.model.selectedCount > 0; implicitWidth: selText.implicitWidth + 20; implicitHeight: 24; radius: 12; color: Theme.secondaryContainer
        MText { id: selText; anchors.centerIn: parent; text: strip.model ? qsTr("%1 selected").arg(strip.model.selectedCount) + (strip.model.selectedSize > 0 ? " · " + Qt.locale().formattedDataSize(strip.model.selectedSize, 1, Locale.DataSizeSIFormat) : "") : ""; role: "labelS"; color: Theme.onSecondaryContainer } }
    Item { Layout.fillWidth: true }
    MText { text: qsTr("Zoom"); role: "labelS"; color: Theme.subtext }
    ExpressiveSlider { compact: true; showIcon: false; Layout.preferredWidth: 120; from: 0; to: 1; stepSize: 0; value: strip.zoom; onMoved: strip.zoomMoved(value) }
}
