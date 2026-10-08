import QtQuick
import QtQml.Models
import Opal.Terminal
// A live session plus what the chrome needs about it: the state of its last command.
Item {
    id: entry
    property int sid: 0
    property TerminalSession session
    property var states: []
    readonly property string status: states.length ? states[states.length - 1] : ""
    Instantiator {
        model: entry.session ? entry.session.marks : null
        delegate: QtObject { required property string state }
        onObjectAdded: (i, o) => { const a = entry.states.slice(); a.splice(i, 0, o.state); entry.states = a; o.stateChanged.connect(() => { const b = entry.states.slice(); b[i] = o.state; entry.states = b; }); }
        onObjectRemoved: (i, o) => { const a = entry.states.slice(); a.splice(i, 1); entry.states = a; }
    }
}
