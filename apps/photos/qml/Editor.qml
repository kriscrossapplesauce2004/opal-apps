import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Opal.Material
import Opal.Photos
// The photo editor: a tool rail (Auto, Crop, Adjust, Filters, Markup), the live preview with a before/after split, and Save.
// The preview runs the same steps as PhotoEditor::render: flip, quarter turns, straighten, crop, adjust, markup.
Rectangle {
    id: ed
    property string path: ""
    property string tool: "adjust"
    property real split: 1
    property bool confirmDiscard: false
    signal closed()
    signal saved(string path, bool asCopy)
    signal message(string text)
    readonly property bool phone: width < 700
    readonly property var info: PhotoLibrary.photo(path)
    readonly property real baseAspect: info.width > 0 && info.height > 0 ? info.width / info.height : (src.implicitHeight > 0 ? src.implicitWidth / src.implicitHeight : 1.5)
    readonly property bool odd: editor.quarterTurns % 2 === 1
    readonly property real frameAspect: odd ? 1 / baseAspect : baseAspect
    // While cropping, the whole frame shows; otherwise only the crop.
    readonly property rect shownCrop: tool === "crop" ? Qt.rect(0, 0, 1, 1) : editor.crop
    readonly property real viewAspect: frameAspect * shownCrop.width / Math.max(.001, shownCrop.height)
    color: "#0b0b0b"
    focus: true
    PhotoEditor {
        id: editor
        path: ed.path
        onSaved: (p, asCopy) => { ed.saved(p, asCopy); ed.closed(); }
        onFailed: m => ed.message(m)
    }
    function cancel() { if (editor.dirty && !confirmDiscard) confirmDiscard = true; else closed(); }
    Keys.onPressed: e => {
        if (e.key === Qt.Key_Escape) { if (draftActive) cropBox.reset(); else cancel(); e.accepted = true; }
        else if (e.key === Qt.Key_Z && e.modifiers & Qt.ControlModifier && tool === "markup") { editor.undoStroke(); e.accepted = true; }
        else if (e.key === Qt.Key_S && e.modifiers & Qt.ControlModifier) { editor.save(!!(e.modifiers & Qt.ShiftModifier)); e.accepted = true; }
        else if (e.key === Qt.Key_Backslash) { split = split < .99 ? 1 : .5; e.accepted = true; }
    }
    readonly property bool draftActive: tool === "crop" && cropBox.changed
    onToolChanged: if (tool !== "crop" && cropBox.changed) cropBox.commit()

    GridLayout {
        anchors.fill: parent
        columns: ed.phone ? 1 : 3; rowSpacing: 0; columnSpacing: 0
        // ---------- tool rail ----------
        Item {
            Layout.preferredWidth: ed.phone ? -1 : 96; Layout.fillHeight: !ed.phone; Layout.fillWidth: ed.phone
            Layout.preferredHeight: ed.phone ? 72 : -1; Layout.row: ed.phone ? 2 : 0; Layout.column: 0
            GridLayout {
                anchors.centerIn: ed.phone ? parent : undefined
                anchors.top: ed.phone ? undefined : parent.top; anchors.topMargin: ed.phone ? 0 : 76
                anchors.horizontalCenter: parent.horizontalCenter
                columns: ed.phone ? 5 : 1; rowSpacing: 10; columnSpacing: 6
                Repeater {
                    model: [{id: "auto", i: "auto_fix_high", t: qsTr("Auto")}, {id: "crop", i: "crop_rotate", t: qsTr("Crop")}, {id: "adjust", i: "tune", t: qsTr("Adjust")},
                            {id: "filters", i: "filter_vintage", t: qsTr("Filters")}, {id: "markup", i: "draw", t: qsTr("Markup")}]
                    ColumnLayout {
                        id: toolItem
                        required property var modelData
                        readonly property bool on: ed.tool === modelData.id
                        spacing: 4
                        Rectangle {
                            Layout.alignment: Qt.AlignHCenter; implicitWidth: 56; implicitHeight: 32; radius: 16
                            color: toolItem.on ? Theme.secondaryContainer : toolHover.hovered ? Qt.rgba(1, 1, 1, .08) : "transparent"
                            Glyph { anchors.centerIn: parent; name: toolItem.modelData.i; fill: toolItem.on ? 1 : 0; color: toolItem.on ? Theme.onSecondaryContainer : "#e6e1d6" }
                        }
                        MText { Layout.alignment: Qt.AlignHCenter; text: toolItem.modelData.t; role: "label"; color: "#e6e1d6" }
                        HoverHandler { id: toolHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: { if (toolItem.modelData.id === "auto") { editor.autoAdjust(); ed.tool = "adjust"; } else ed.tool = toolItem.modelData.id; } }
                        Accessible.role: Accessible.PageTab; Accessible.name: modelData.t; Accessible.selected: on
                    }
                }
            }
        }
        // ---------- stage ----------
        Item {
            id: stageArea
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.row: 0; Layout.column: ed.phone ? 0 : 1
            Image {
                id: src
                visible: false
                source: ed.path ? PhotoLibrary.uri(ed.path) : ""
                asynchronous: true; autoTransform: true
                sourceSize: Qt.size(2400, 2400)
            }
            Item {
                id: view
                readonly property real room: ed.tool === "crop" ? 56 : 24
                width: Math.min(parent.width - room * 2, (parent.height - 96 - room) * ed.viewAspect)
                height: width / ed.viewAspect
                anchors.horizontalCenter: parent.horizontalCenter
                y: 72 + (parent.height - 72 - 24 - height) / 2
                // Geometry: the frame is scaled so the crop fills the view.
                Item {
                    id: geo
                    anchors.fill: parent; clip: true
                    Item {
                        id: frame
                        width: view.width / ed.shownCrop.width; height: view.height / ed.shownCrop.height
                        x: -ed.shownCrop.x * width; y: -ed.shownCrop.y * height
                        Item {
                            anchors.fill: parent
                            rotation: editor.straighten
                            scale: editor.coverScale(frame.width, frame.height, editor.straighten)
                            Item {
                                anchors.centerIn: parent
                                width: ed.odd ? frame.height : frame.width; height: ed.odd ? frame.width : frame.height
                                rotation: editor.quarterTurns * 90
                                Image {
                                    anchors.fill: parent; source: src.source; sourceSize: src.sourceSize; asynchronous: true; autoTransform: true
                                    fillMode: Image.Stretch; smooth: true; mipmap: true
                                    transform: Scale { origin.x: parent.width / 2; xScale: editor.flipped ? -1 : 1 }
                                }
                            }
                        }
                    }
                }
                ShaderEffectSource { id: geoSource; sourceItem: geo; hideSource: true; live: true; smooth: true }
                // After: the adjustments on the GPU, the same maths as the save.
                ShaderEffect {
                    anchors.fill: parent
                    property variant source: geoSource
                    readonly property var e: editor.effective
                    property real brightness: e.brightness || 0
                    property real contrast: e.contrast || 0
                    property real saturation: e.saturation || 0
                    property real warmth: e.warmth || 0
                    property real tint: e.tint || 0
                    property real highlights: e.highlights || 0
                    property real shadows: e.shadows || 0
                    property real vignette: e.vignette || 0
                    property real fade: e.fade || 0
                    fragmentShader: "qrc:/opal/photos/shaders/adjust.frag.qsb"
                }
                // Before: the left part of the split shows the photo without adjustments.
                Item {
                    visible: ed.split < .995 && ed.tool !== "crop"
                    width: view.width * ed.split; height: view.height; clip: true
                    ShaderEffectSource { width: view.width; height: view.height; sourceItem: geo; live: true }
                }
                Rectangle {
                    visible: ed.split < .995 && ed.tool !== "crop"
                    x: view.width * ed.split - 1; width: 2; height: view.height; color: "#ffffff"
                    Rectangle { anchors.centerIn: parent; width: 40; height: 40; radius: 20; color: "#ffffff"
                        Glyph { anchors.centerIn: parent; name: "compare_arrows"; color: "#111111" }
                        DragHandler { target: null; xAxis.enabled: true; yAxis.enabled: false
                            onCentroidChanged: if (active) ed.split = Math.max(0, Math.min(1, view.mapFromItem(parent, centroid.position.x, 0).x / view.width)) } } }
                Repeater {
                    model: ed.split < .995 && ed.tool !== "crop" ? [{t: qsTr("Before"), left: true}, {t: qsTr("After"), left: false}] : []
                    Rectangle { required property var modelData; x: modelData.left ? 12 : view.width - width - 12; y: 12; implicitWidth: lbl.implicitWidth + 22; implicitHeight: 28; radius: 14; color: Qt.rgba(0, 0, 0, .55)
                        MText { id: lbl; anchors.centerIn: parent; text: parent.modelData.t; role: "labelS"; color: "#ffffff" } }
                }
                // Markup: strokes are normalised to the crop, so they stay where they were drawn.
                Canvas {
                    id: ink
                    anchors.fill: parent
                    visible: ed.tool !== "crop"
                    property var live: []
                    onPaint: {
                        const c = getContext("2d"); c.reset();
                        const all = editor.strokes.concat(live.length > 1 ? [{colour: markup.colour, width: markup.penWidth, highlighter: markup.highlighter, points: live}] : []);
                        for (const s of all) {
                            const p = s.points; if (p.length < 4) continue;
                            c.globalAlpha = s.highlighter ? .4 : 1;
                            c.strokeStyle = s.colour; c.lineWidth = s.width * width; c.lineCap = s.highlighter ? "butt" : "round"; c.lineJoin = "round";
                            c.beginPath(); c.moveTo(p[0] * width, p[1] * height);
                            for (let i = 2; i + 1 < p.length; i += 2) c.lineTo(p[i] * width, p[i + 1] * height);
                            c.stroke();
                        }
                    }
                    Connections { target: editor; function onStrokesChanged() { ink.requestPaint(); } }
                    onWidthChanged: requestPaint()
                    DragHandler {
                        enabled: ed.tool === "markup"; target: null; dragThreshold: 0
                        onActiveChanged: {
                            if (active) ink.live = [];
                            else if (ink.live.length > 3) { editor.strokes = editor.strokes.concat([{colour: markup.colour, width: markup.penWidth, highlighter: markup.highlighter, points: ink.live}]); ink.live = []; }
                        }
                        onCentroidChanged: if (active) { const p = centroid.position; ink.live = ink.live.concat([Math.max(0, Math.min(1, p.x / ink.width)), Math.max(0, Math.min(1, p.y / ink.height))]); ink.requestPaint(); }
                    }
                    HoverHandler { enabled: ed.tool === "markup"; cursorShape: Qt.CrossCursor }
                }
                // Crop box over the whole frame, with corner handles and a rule-of-thirds grid.
                Item {
                    id: cropBox
                    visible: ed.tool === "crop"
                    anchors.fill: parent
                    property rect r: editor.crop
                    property bool changed: false
                    property real lock: 0
                    function reset() { r = editor.crop; changed = false; }
                    function commit() { editor.crop = r; changed = false; }
                    function setRatio(ratio) {
                        lock = ratio;
                        if (ratio <= 0) return;
                        // The largest centred box of that ratio in pixels.
                        const fa = ed.frameAspect; let w = 1, h = fa / ratio;
                        if (h > 1) { h = 1; w = ratio / fa; }
                        r = Qt.rect((1 - w) / 2, (1 - h) / 2, w, h); changed = true;
                    }
                    Connections { target: editor; function onGeometryChanged() { cropBox.r = editor.crop; } }
                    readonly property real bx: r.x * width
                    readonly property real by: r.y * height
                    readonly property real bw: r.width * width
                    readonly property real bh: r.height * height
                    // Shade outside the crop.
                    Rectangle { x: 0; y: 0; width: parent.width; height: cropBox.by; color: Qt.rgba(0, 0, 0, .55) }
                    Rectangle { x: 0; y: cropBox.by + cropBox.bh; width: parent.width; height: parent.height - y; color: Qt.rgba(0, 0, 0, .55) }
                    Rectangle { x: 0; y: cropBox.by; width: cropBox.bx; height: cropBox.bh; color: Qt.rgba(0, 0, 0, .55) }
                    Rectangle { x: cropBox.bx + cropBox.bw; y: cropBox.by; width: parent.width - x; height: cropBox.bh; color: Qt.rgba(0, 0, 0, .55) }
                    Rectangle {
                        x: cropBox.bx; y: cropBox.by; width: cropBox.bw; height: cropBox.bh
                        color: "transparent"; border.width: 2; border.color: "#ffffff"
                        Repeater { model: 2
                            Rectangle { required property int index; x: parent.width * (index + 1) / 3; width: 1; height: parent.height; color: Qt.rgba(1, 1, 1, .45) } }
                        Repeater { model: 2
                            Rectangle { required property int index; y: parent.height * (index + 1) / 3; height: 1; width: parent.width; color: Qt.rgba(1, 1, 1, .45) } }
                        DragHandler {
                            target: null
                            property rect start
                            onActiveChanged: if (active) start = cropBox.r
                            onTranslationChanged: if (active) {
                                const dx = translation.x / cropBox.width, dy = translation.y / cropBox.height;
                                cropBox.r = Qt.rect(Math.max(0, Math.min(1 - start.width, start.x + dx)), Math.max(0, Math.min(1 - start.height, start.y + dy)), start.width, start.height);
                                cropBox.changed = true;
                            }
                        }
                    }
                    Repeater {
                        model: [[0, 0], [1, 0], [0, 1], [1, 1]]
                        Rectangle {
                            id: handle
                            required property var modelData
                            x: cropBox.bx + modelData[0] * cropBox.bw - 12; y: cropBox.by + modelData[1] * cropBox.bh - 12
                            width: 24; height: 24; radius: 12; color: "#ffffff"; border.width: 3; border.color: Theme.primary
                            DragHandler {
                                target: null
                                property rect start
                                onActiveChanged: if (active) start = cropBox.r
                                onTranslationChanged: if (active) {
                                    const dx = translation.x / cropBox.width, dy = translation.y / cropBox.height, m = handle.modelData, min = .05;
                                    let x0 = start.x, y0 = start.y, x1 = start.x + start.width, y1 = start.y + start.height;
                                    if (m[0] === 0) x0 = Math.max(0, Math.min(x1 - min, x0 + dx)); else x1 = Math.min(1, Math.max(x0 + min, x1 + dx));
                                    if (m[1] === 0) y0 = Math.max(0, Math.min(y1 - min, y0 + dy)); else y1 = Math.min(1, Math.max(y0 + min, y1 + dy));
                                    // A locked ratio follows the width.
                                    if (cropBox.lock > 0) { const h = (x1 - x0) * ed.frameAspect / cropBox.lock; if (m[1] === 0) y0 = Math.max(0, y1 - h); else y1 = Math.min(1, y0 + h); }
                                    cropBox.r = Qt.rect(x0, y0, x1 - x0, y1 - y0); cropBox.changed = true;
                                }
                            }
                            Accessible.role: Accessible.Grip; Accessible.name: qsTr("Crop corner")
                        }
                    }
                }
            }
            // ---------- top actions ----------
            RowLayout {
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
                spacing: 8
                MButton { text: qsTr("Cancel"); contained: true; ink: "#ffffff"; onClicked: ed.cancel() }
                Item { Layout.fillWidth: true }
                MButton { visible: ed.tool !== "crop" && !ed.phone; icon: "compare"; compact: true; tooltip: qsTr("Compare with the original"); ink: "#ffffff"; checked: ed.split < .99; onClicked: ed.split = ed.split < .99 ? 1 : .5 }
                MButton { visible: editor.dirty; icon: "restart_alt"; compact: true; tooltip: qsTr("Undo all edits"); ink: "#ffffff"; onClicked: { editor.reset(); cropBox.reset(); } }
                MButton { text: qsTr("Save copy"); tonal: true; enabled: editor.dirty && !editor.busy; onClicked: { if (cropBox.changed) cropBox.commit(); editor.save(true); } }
                MButton { text: editor.busy ? qsTr("Saving…") : qsTr("Save"); filled: true; enabled: editor.dirty && !editor.busy; onClicked: { if (cropBox.changed) cropBox.commit(); editor.save(false); } }
            }
            // Discard check for Cancel with edits.
            Rectangle {
                visible: ed.confirmDiscard
                anchors.horizontalCenter: parent.horizontalCenter; y: 72
                width: dRow.implicitWidth + 32; height: 56; radius: 28; color: Theme.inverseSurface; z: 5
                RowLayout { id: dRow; anchors.centerIn: parent; spacing: 10
                    MText { text: qsTr("Discard your edits?"); color: Theme.inverseInk }
                    MButton { text: qsTr("Keep editing"); compact: true; ink: Theme.inverseInk; onClicked: ed.confirmDiscard = false }
                    MButton { text: qsTr("Discard"); compact: true; ink: Theme.inversePrimary; onClicked: ed.closed() } }
            }
        }
        // ---------- tool panel ----------
        Rectangle {
            Layout.preferredWidth: ed.phone ? -1 : 330; Layout.fillHeight: !ed.phone; Layout.fillWidth: ed.phone
            Layout.preferredHeight: ed.phone ? Math.min(300, ed.height * .4) : -1
            Layout.row: ed.phone ? 1 : 0; Layout.column: ed.phone ? 0 : 2
            color: Theme.surfaceLow
            Flickable {
                anchors.fill: parent; anchors.margins: 20; contentHeight: panelCol.implicitHeight; clip: true; boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                ColumnLayout {
                    id: panelCol
                    width: parent.width; spacing: 8
                    MText { text: ({auto: qsTr("Adjust"), adjust: qsTr("Adjust"), crop: qsTr("Crop and rotate"), filters: qsTr("Filters"), markup: qsTr("Markup")})[ed.tool]; role: "title" }
                    // Adjust: one expressive slider per adjustment, centred at zero.
                    Repeater {
                        model: ed.tool === "adjust" ? editor.adjustments : []
                        ColumnLayout {
                            required property var modelData
                            readonly property real v: editor.values[modelData.id] || 0
                            Layout.fillWidth: true; spacing: 0
                            RowLayout { Layout.fillWidth: true
                                MText { text: modelData.name; role: "label"; Layout.fillWidth: true }
                                MText { text: (parent.parent.v > 0 ? "+" : "") + Math.round(parent.parent.v * 100); role: "labelS"; color: Theme.subtext } }
                            ExpressiveSlider {
                                Layout.fillWidth: true; icon: modelData.icon; implicitHeight: 46; trackHeight: 28
                                from: modelData.id === "fade" ? 0 : -100; to: 100; value: parent.v * 100
                                onMoved: editor.setValue(modelData.id, value / 100)
                                Accessible.name: modelData.name
                            }
                        }
                    }
                    MButton { visible: ed.tool === "adjust"; Layout.fillWidth: true; Layout.topMargin: 6; text: qsTr("Auto"); icon: "auto_fix_high"; tonal: true; onClicked: editor.autoAdjust() }
                    // Crop: turns, flip, straighten and ratio.
                    ColumnLayout {
                        visible: ed.tool === "crop"
                        Layout.fillWidth: true; spacing: 12
                        RowLayout { spacing: 8
                            MButton { icon: "rotate_left"; tonal: true; tooltip: qsTr("Rotate left"); onClicked: editor.quarterTurns = (editor.quarterTurns + 3) % 4 }
                            MButton { icon: "rotate_right"; tonal: true; tooltip: qsTr("Rotate right"); onClicked: editor.quarterTurns = (editor.quarterTurns + 1) % 4 }
                            MButton { icon: "flip"; tonal: true; checked: editor.flipped; tooltip: qsTr("Mirror"); onClicked: editor.flipped = !editor.flipped } }
                        RowLayout { Layout.fillWidth: true
                            MText { text: qsTr("Straighten"); role: "label"; Layout.fillWidth: true }
                            MText { text: editor.straighten.toFixed(1) + "°"; role: "labelS"; color: Theme.subtext } }
                        ExpressiveSlider { Layout.fillWidth: true; icon: "straighten"; implicitHeight: 46; trackHeight: 28; from: -45; to: 45; stepSize: .5; value: editor.straighten; onMoved: editor.straighten = value; Accessible.name: qsTr("Straighten") }
                        MText { text: qsTr("Shape"); role: "label"; Layout.topMargin: 4 }
                        Flow { Layout.fillWidth: true; spacing: 6
                            Repeater { model: [{t: qsTr("Free"), r: 0}, {t: qsTr("Original"), r: -1}, {t: "1:1", r: 1}, {t: "4:3", r: 4 / 3}, {t: "3:2", r: 1.5}, {t: "16:9", r: 16 / 9}, {t: "9:16", r: 9 / 16}]
                                Chip { required property var modelData; text: modelData.t; glass: false
                                    selected: modelData.r === -1 ? Math.abs(cropBox.lock - ed.frameAspect) < .001 : cropBox.lock === modelData.r
                                    onClicked: cropBox.setRatio(modelData.r === -1 ? ed.frameAspect : modelData.r) } } }
                        RowLayout { Layout.topMargin: 6; spacing: 8
                            MButton { text: qsTr("Reset crop"); icon: "crop_free"; onClicked: { cropBox.r = Qt.rect(0, 0, 1, 1); cropBox.lock = 0; cropBox.changed = true; } }
                            MButton { visible: cropBox.changed; text: qsTr("Done"); filled: true; onClicked: { cropBox.commit(); ed.tool = "adjust"; } } }
                    }
                    // Filters: a card per look, with the photo under its name.
                    GridLayout {
                        visible: ed.tool === "filters"
                        Layout.fillWidth: true; columns: 2; columnSpacing: 10; rowSpacing: 10
                        Repeater {
                            model: ed.tool === "filters" ? editor.filters : []
                            ColumnLayout {
                                required property var modelData
                                readonly property bool on: editor.filter === modelData.id
                                Layout.fillWidth: true; spacing: 4
                                RoundedImage {
                                    Layout.fillWidth: true; Layout.preferredHeight: width * .7; radius: parent.on ? 22 : 14
                                    source: ed.path ? PhotoLibrary.thumb(ed.path) : ""; sourceSize: Qt.size(260, 180)
                                    Rectangle { anchors.fill: parent; radius: parent.radius; color: "transparent"; border.width: parent.parent.on ? 3 : 0; border.color: Theme.primary }
                                    TapHandler { onTapped: editor.filter = parent.parent.modelData.id }
                                }
                                MText { text: modelData.name; role: "label"; color: parent.on ? Theme.primary : Theme.text }
                                Accessible.role: Accessible.RadioButton; Accessible.name: modelData.name; Accessible.checked: on
                            }
                        }
                    }
                    // Markup: pen colour, size and highlighter.
                    QtObject { id: markup; property color colour: "#ff5a5f"; property real penWidth: .012; property bool highlighter: false }
                    ColumnLayout {
                        visible: ed.tool === "markup"
                        Layout.fillWidth: true; spacing: 12
                        SegmentedButtons { current: markup.highlighter ? "hl" : "pen"; options: [{id: "pen", name: qsTr("Pen"), icon: "edit"}, {id: "hl", name: qsTr("Highlighter"), icon: "ink_highlighter"}]; onPicked: id => markup.highlighter = id === "hl" }
                        Flow { Layout.fillWidth: true; spacing: 10
                            Repeater { model: ["#ff5a5f", "#ffb020", "#ffe14d", "#34c77b", "#3d9df2", "#a46cf5", "#ffffff", "#111111"]
                                Rectangle { required property string modelData; width: 34; height: 34; radius: 17; color: modelData
                                    border.width: Qt.colorEqual(markup.colour, modelData) ? 3 : 1; border.color: Qt.colorEqual(markup.colour, modelData) ? Theme.primary : Theme.alpha(Theme.text, .2)
                                    TapHandler { onTapped: markup.colour = parent.modelData }
                                    Accessible.role: Accessible.RadioButton; Accessible.name: modelData } } }
                        MText { text: qsTr("Size"); role: "label" }
                        ExpressiveSlider { Layout.fillWidth: true; icon: "line_weight"; implicitHeight: 46; trackHeight: 28; from: 2; to: 60; value: markup.penWidth * 1000; onMoved: markup.penWidth = value / 1000; Accessible.name: qsTr("Pen size") }
                        RowLayout { spacing: 8
                            MButton { text: qsTr("Undo"); icon: "undo"; enabled: editor.strokes.length > 0; onClicked: editor.undoStroke() }
                            MButton { text: qsTr("Clear"); icon: "ink_eraser"; enabled: editor.strokes.length > 0; onClicked: editor.strokes = [] } }
                    }
                }
            }
        }
    }
}
