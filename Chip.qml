import QtQuick
import QtQuick.Controls.Basic

// Selection is driven from outside (bound to the device value), so the button never toggles itself.
AbstractButton {
    id: c
    implicitWidth: label.implicitWidth + 28; implicitHeight: 30
    hoverEnabled: true
    contentItem: Text { id: label; text: c.text; font.pixelSize: 13; font.family: Theme.font
        color: c.checked ? Theme.a[100] : Theme.n[200]
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
    background: Rectangle { radius: Theme.radius
        color: c.checked ? Theme.a[900] : "transparent"
        border.width: 1; border.color: c.checked || c.hovered ? Theme.accent : Theme.n[700] }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
