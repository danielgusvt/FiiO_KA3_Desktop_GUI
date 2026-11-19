import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Placeholder impulse responses ported from the design reference; swap in real data later.
AbstractButton {
    id: c
    property int curveIndex: 0
    hoverEnabled: true
    padding: 12
    implicitHeight: 100

    function curve() {
        const fc = [0.35, 0.12, 0.3, 0.14, 0.35, 0.2, 0.4][curveIndex]
        const dec = [0.01, 0.08, 0.035, 0.1, 0.012, 0.06, 0.006][curveIndex]
        const ctr = [0.5, 0.2, 0.18, 0.15, 0.5, 0.28, 0.5][curveIndex]
        const pts = []
        for (let x = 0; x <= 200; x += 2) {
            const u = x / 200 - ctr, t = u * 70, a = t * fc * Math.PI * 2
            const sc = a === 0 ? 1 : Math.sin(a) / a
            const env = Math.exp(-Math.abs(u) / (dec * 6))
            const k = (curveIndex === 1 || curveIndex === 3) ? Math.exp(-Math.abs(t) / 6) : env + 0.1 * (1 - Math.abs(u))
            pts.push(Math.max(2, Math.min(48, 25 - 22 * sc * k)))
        }
        return pts
    }

    background: Rectangle { radius: Theme.radius
        color: c.checked ? Theme.a[900] : "transparent"
        border.width: 1; border.color: c.checked ? Theme.accent : (c.hovered ? Theme.a[500] : Theme.n[800]) }
    contentItem: ColumnLayout {
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            Text { Layout.fillWidth: true; text: c.text; font.pixelSize: 13; font.family: Theme.font
                   color: c.checked ? Theme.a[100] : Theme.text; elide: Text.ElideRight }
            Icon { name: c.checked ? "check-circle-fill" : "circle"; size: 16
                   color: c.checked ? Theme.accent : Theme.n[500] }
        }
        Canvas {
            id: cv
            Layout.fillWidth: true; Layout.preferredHeight: 50
            property color stroke: c.checked ? Theme.a[200] : Theme.n[300]
            property var pts: c.curve()
            onStrokeChanged: requestPaint()
            onWidthChanged: requestPaint()
            onPaint: {
                const g = getContext("2d")
                g.reset()
                g.strokeStyle = stroke
                g.lineWidth = 1.4
                g.beginPath()
                for (let i = 0; i < pts.length; ++i) {
                    const x = i / (pts.length - 1) * width
                    i ? g.lineTo(x, pts[i]) : g.moveTo(x, pts[i])
                }
                g.stroke()
            }
        }
    }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
