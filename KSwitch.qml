import QtQuick
import QtQuick.Controls.Basic

Switch {
    id: c
    implicitWidth: 40; implicitHeight: 22
    indicator: Rectangle {
        anchors.fill: parent; radius: 11
        color: c.checked ? Theme.a[800] : "transparent"
        border.color: Theme.accent; border.width: 1
        Rectangle {
            width: 16; height: 16; radius: 8; y: 3
            x: c.checked ? 21 : 3
            color: c.checked ? Theme.accent : Theme.n[500]
            Behavior on x { NumberAnimation { duration: 150 } }
        }
    }
    contentItem: Item {}
    background: null
    focusPolicy: Qt.TabFocus
    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
