import QtQuick
import QtQuick.Controls.Basic

// `source` is the value the device reports. It is followed while the user is not interacting;
// user changes are debounced and delivered through committed().
Slider {
    id: c
    property real source: 0
    property int debounce: 30
    property real resetValue: NaN   // double-click target, if any
    signal committed(int v)

    function reset() {
        timer.stop()
        value = resetValue
        committed(Math.round(value))
    }

    stepSize: 1; snapMode: Slider.SnapAlways
    implicitHeight: 20

    Binding { target: c; property: "value"; value: c.source; when: !c.pressed && !timer.running }
    Timer { id: timer; interval: c.debounce; onTriggered: c.committed(Math.round(c.value)) }
    onMoved: timer.restart()
    onPressedChanged: if (!pressed && timer.running) { timer.stop(); committed(Math.round(value)) }

    TapHandler { onDoubleTapped: if (!isNaN(c.resetValue)) c.reset() }

    background: Rectangle {
        x: c.leftPadding; y: c.topPadding + c.availableHeight / 2 - 2
        width: c.availableWidth; height: 4; radius: 2; color: Theme.n[700]
        Rectangle { width: c.visualPosition * parent.width; height: parent.height; radius: 2; color: Theme.accent }
    }
    handle: Rectangle {
        x: c.leftPadding + c.visualPosition * (c.availableWidth - width)
        y: c.topPadding + c.availableHeight / 2 - height / 2
        width: 16; height: 16; radius: 8
        color: c.pressed || c.hovered ? Theme.a[400] : Theme.accent
        border.color: Theme.a[200]; border.width: 1
        Rectangle { anchors.fill: parent; anchors.margins: -4; radius: width / 2; color: "transparent"
                    border.color: Theme.accent; border.width: 2; visible: c.visualFocus }
    }
}
