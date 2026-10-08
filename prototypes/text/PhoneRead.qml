import QtQuick
import QtQuick.Layouts
import Opal.Material
// Read mode on a phone: EPUB text reflows to the screen, a slim progress line, and a type chip for font and size.
Rectangle {
    color: Theme.mix(Theme.base, "#2a2418", .35)
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 22; anchors.topMargin: 12; spacing: 14
        RowLayout { Layout.fillWidth: true
            Glyph { name: "arrow_back"; color: Theme.text }
            MText { text: "Frankenstein"; role: "titleS"; Layout.leftMargin: 10; Layout.fillWidth: true }
            Rectangle { implicitHeight: 34; implicitWidth: 52; radius: 17; color: Theme.surfaceHigh; MText { anchors.centerIn: parent; text: "Aa"; role: "label"; font.weight: Font.Bold } }
            Glyph { name: "bookmark"; color: Theme.text; Layout.leftMargin: 10 } }
        MText { text: "LETTER 1"; role: "label"; color: Theme.primary; Layout.topMargin: 10 }
        MText { text: "To Mrs. Saville, England"; font.pixelSize: 26; font.weight: Font.Bold; font.variableAxes: ({"wdth": 110, "ROND": 40}) }
        MText { text: "St. Petersburgh, Dec. 11th, 17—"; role: "bodyS"; color: Theme.subtext; font.italic: true }
        MText { Layout.fillWidth: true; wrapMode: Text.WordWrap; font.family: "serif"; font.pixelSize: 18; lineHeight: 1.45; color: Theme.mix(Theme.text, Theme.subtext, .2)
            text: "You will rejoice to hear that no disaster has accompanied the commencement of an enterprise which you have regarded with such evil forebodings. I arrived here yesterday, and my first task is to assure my dear sister of my welfare and increasing confidence in the success of my undertaking.\n\nI am already far north of London, and as I walk in the streets of Petersburgh, I feel a cold northern breeze play upon my cheeks, which braces my nerves and fills me with delight." }
        Item { Layout.fillHeight: true }
        RowLayout { Layout.fillWidth: true; spacing: 10
            MText { text: "Letter 1"; role: "labelS"; color: Theme.subtext }
            WavyProgress { Layout.fillWidth: true; value: .04; animate: false; amplitude: 2; wavelength: 14; thickness: 3 }
            MText { text: "12 min left in chapter"; role: "labelS"; color: Theme.subtext } }
    }
}
