import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: c
    property string label
    property string hint
    property int labelSize: 14
    property bool separator: true
    property alias checked: sw.checked
    signal toggled(bool v)

    Layout.fillWidth: true
    spacing: 16

    RowLayout {
        Layout.fillWidth: true
        spacing: 16
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            Text { text: c.label; color: Theme.text; font.pixelSize: c.labelSize; font.family: Theme.font }
            Hint { text: c.hint; Layout.fillWidth: true }
        }
        KSwitch { id: sw; Layout.alignment: Qt.AlignVCenter; onToggled: c.toggled(checked) }
    }
    Rectangle { visible: c.separator; Layout.fillWidth: true; height: 1; color: Theme.n[800] }
}
