import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: c
    property string label
    property string hint
    property var options: []
    property int current: 0
    signal picked(int i)

    Layout.fillWidth: true
    spacing: 8

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0
        Text { text: c.label; color: Theme.text; font.pixelSize: 14; font.family: Theme.font }
        Hint { visible: text !== ""; text: c.hint; Layout.fillWidth: true }
    }
    Flow {
        Layout.fillWidth: true
        spacing: 8
        Repeater {
            model: c.options
            Chip { text: modelData; checked: index === c.current; onClicked: c.picked(index) }
        }
    }
    Item { implicitHeight: 8 }
    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.n[800] }
}
