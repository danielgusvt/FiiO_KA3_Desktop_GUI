import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtCore

// `device` is the DeviceBackend, exposed as a context property from main.cpp.
ApplicationWindow {
    id: win
    width: 900; height: 600; minimumWidth: 760; minimumHeight: 520; visible: true
    title: "FiiO KA3 Control"
    color: Theme.bg
    font.family: Theme.font

    property int page: 0
    readonly property bool fineVolume: settings.fineVolume
    readonly property int volScale: fineVolume ? 1 : 2     // device units per slider step
    readonly property int balanceMax: 12                   // per-channel attenuation steps

    Settings { id: settings; category: "ui"; property bool fineVolume: false }

    // Write failures show for a few seconds in the footer; the backend re-reads the device to revert controls.
    Connections {
        target: device
        function onWriteFailed(what) { errorText.text = what; errorTimer.restart() }
    }
    Timer { id: errorTimer; interval: 5000; onTriggered: errorText.text = "" }

    RowLayout {
        anchors.fill: parent; spacing: 0

        // sidebar -------------------------------------------------------------
        Rectangle {
            Layout.preferredWidth: 232; Layout.fillHeight: true
            gradient: Gradient { GradientStop { position: 0; color: Theme.n[900] } GradientStop { position: 1; color: Theme.bg } }
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.n[800] }
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 16; anchors.topMargin: 20; spacing: 20
                ColumnLayout { spacing: 4
                    RowLayout { spacing: 6
                        Item { width: 14; height: 14
                            Rectangle { anchors.centerIn: parent; width: 14; height: 14; radius: 7
                                color: Theme.accent; opacity: 0.18; visible: device.connected }
                            Rectangle { anchors.centerIn: parent; width: 10; height: 10; radius: 5
                                color: Theme.accent; opacity: 0.3; visible: device.connected }
                            Rectangle { anchors.centerIn: parent; width: 6; height: 6; radius: 3
                                color: device.connected ? Theme.accent : Theme.n[600] } }
                        Text { text: device.connected ? "CONNECTED" : "DISCONNECTED"; color: Theme.n[400]
                               font.pixelSize: 11; font.letterSpacing: 0.9 } }
                    Text { text: "FiiO KA3"; color: Theme.text; font.pixelSize: 22; font.weight: Font.Medium }
                    Hint { text: device.connected ? "Firmware " + device.firmware : "Firmware —" } }
                Rectangle { Layout.fillWidth: true; implicitHeight: sr.implicitHeight + 24; radius: 8
                    color: "transparent"; border.color: Theme.n[800]
                    ColumnLayout { id: sr; anchors.fill: parent; anchors.margins: 12; spacing: 2
                        Text { text: "SAMPLE RATE"; color: Theme.n[400]; font.pixelSize: 11; font.letterSpacing: 0.9 }
                        // "44.1kHz" → big number + small unit; labels like "DoP64" are shown whole.
                        readonly property var parts: /^([\d.]+)\s*([A-Za-z]+)$/.exec(device.sampleRateLabel)
                        RowLayout { spacing: 4
                            Text { text: !device.connected || device.sampleRateLabel === "" ? "—"
                                       : sr.parts ? sr.parts[1] : device.sampleRateLabel
                                   color: Theme.text; font.weight: Font.Medium
                                   font.pixelSize: sr.parts || !device.connected ? 26 : 20 }
                            Text { visible: device.connected && sr.parts; text: sr.parts ? sr.parts[2] : ""
                                   color: Theme.n[300]; font.pixelSize: 14
                                   Layout.alignment: Qt.AlignBottom; Layout.bottomMargin: 4 } }
                        Hint { text: "Read-only · live" } } }
                ColumnLayout { spacing: 2
                    Repeater { model: [{t: "Device", i: "sliders"}, {t: "Audio", i: "waveform"}, {t: "Filters", i: "chart-line"}]
                        delegate: AbstractButton { id: nav
                            readonly property bool active: win.page === index
                            Layout.fillWidth: true; implicitHeight: 36; hoverEnabled: true
                            onClicked: win.page = index
                            background: Rectangle { radius: 8
                                color: nav.active ? Theme.a[900] : (nav.hovered ? Theme.n[800] : "transparent") }
                            contentItem: RowLayout { spacing: 10
                                Item { Layout.leftMargin: 10; implicitWidth: 18; implicitHeight: 18
                                    Icon { anchors.fill: parent; name: modelData.i
                                           color: nav.active ? Theme.a[200] : Theme.n[300] } }
                                Text { text: modelData.t; Layout.fillWidth: true; verticalAlignment: Text.AlignVCenter
                                       font.pixelSize: 14; color: nav.active ? Theme.a[200] : Theme.n[300] } }
                            HoverHandler { cursorShape: Qt.PointingHandCursor } } } }
                Item { Layout.fillHeight: true }
            }
        }

        // content -------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 24; Layout.leftMargin: 28; Layout.rightMargin: 28; spacing: 20
            Text { text: ["Device", "Audio", "Lowpass filter"][win.page]; color: Theme.text; font.pixelSize: 20; font.weight: Font.Medium }

            Item { Layout.fillWidth: true; Layout.fillHeight: true
              StackLayout { id: stack; anchors.fill: parent; currentIndex: win.page
                enabled: device.connected; opacity: device.connected ? 1 : 0.45

                // Device -----------------------------------------------------
                ScrollView { id: devScroll; clip: true; contentWidth: availableWidth
                    ColumnLayout { width: devScroll.availableWidth; spacing: 18
                        ToggleRow { label: "Hardware mute switch"
                            hint: "The device is embedded with a hardware mute switch, which can mute the impulsive sound that occurs when switching between tracks of different sampling rates. However, when the mute switch functions, it will also make a slight pop sound. You may choose to enable or disable it according to your preference."
                            checked: device.hardwareMute; onToggled: v => device.hardwareMute = v }
                        ToggleRow { label: "Dynamic power policy"
                            hint: "With this enabled, the system will dynamically adjust the power policy according to the playback sampling rates, so when playing tracks with lower sampling rates, the power consumption and heat generation will be effectively reduced."
                            checked: device.dynamicPower; onToggled: v => device.dynamicPower = v }
                        ChipGroup { label: "Indicator lights"; hint: "Turn off lasts until restart or always"
                            options: ["On", "Off until restart", "Off always"]; current: device.indicator
                            onPicked: i => device.indicator = i }
                        ChipGroup { label: "Impedance"
                            hint: "The output impedance setting can help match the amplifier with different headphones for better sound quality. When the headphone impedance is below 120Ω, select the low impedance output, otherwise select the high impedance output."
                            options: ["Low", "High"]; current: device.impedance; onPicked: i => device.impedance = i }
                        ChipGroup { label: "Gain"; options: ["Low", "High"]; current: device.gain; onPicked: i => device.gain = i }
                    } }

                // Audio ------------------------------------------------------
                ScrollView { id: audioScroll; clip: true; contentWidth: availableWidth
                  ColumnLayout { width: audioScroll.availableWidth; spacing: 28
                    ColumnLayout { spacing: 10; Layout.fillWidth: true
                        RowLayout { Layout.fillWidth: true; spacing: 16
                            ColumnLayout { Layout.fillWidth: true; spacing: 0
                                Text { text: "Volume"; color: Theme.text; font.pixelSize: 14 }
                                Hint { Layout.fillWidth: true
                                    text: "This setting specifies the max output volume limit of the amplifier. When adjusting volume on the audio source, the volume is divided by steps under this limit." } }
                            Text { text: Math.round(volSlider.value); color: Theme.text; font.pixelSize: 24; font.weight: Font.Medium
                                   Layout.alignment: Qt.AlignTop }
                            Hint { text: " / " + volSlider.to; Layout.alignment: Qt.AlignTop; Layout.topMargin: 8 } }
                        KSlider { id: volSlider; Layout.fillWidth: true
                            from: 0; to: 240 / win.volScale
                            source: Math.round(device.volume / win.volScale)
                            onCommitted: v => device.volume = v * win.volScale }
                        RowLayout { Hint { text: "0"; color: Theme.n[500]; font.pixelSize: 11 } Item { Layout.fillWidth: true }
                                    Hint { text: volSlider.to / 2; color: Theme.n[500]; font.pixelSize: 11 } Item { Layout.fillWidth: true }
                                    Hint { text: volSlider.to; color: Theme.n[500]; font.pixelSize: 11 } }
                        // The device value is untouched when switching, so the loudness stays the same.
                        ToggleRow { label: "Fine volume steps"; labelSize: 13; separator: false
                            hint: "Use the full 0–240 device range instead of steps of 2"
                            checked: win.fineVolume; onToggled: v => settings.fineVolume = v }
                    }
                    // One slider per channel; the device stores attenuation, so the slider is a level
                    // (full right = no attenuation).
                    ColumnLayout { spacing: 14; Layout.fillWidth: true
                        Text { text: "Channel balance"; color: Theme.text; font.pixelSize: 14 }
                        Repeater { model: [{name: "Left"}, {name: "Right"}]
                            delegate: ColumnLayout { id: ch
                                readonly property bool isLeft: index === 0
                                readonly property int attenuation: win.balanceMax - Math.round(slider.value)
                                spacing: 6; Layout.fillWidth: true
                                RowLayout { Text { text: modelData.name; color: Theme.text; font.pixelSize: 14 }
                                    Item { Layout.fillWidth: true }
                                    Text { text: ch.attenuation === 0 ? "0" : "−" + ch.attenuation
                                           color: Theme.text; font.pixelSize: 24; font.weight: Font.Medium } }
                                KSlider { id: slider; Layout.fillWidth: true
                                    from: 0; to: win.balanceMax; resetValue: win.balanceMax
                                    source: win.balanceMax - (ch.isLeft ? device.balanceLeft : device.balanceRight)
                                    onCommitted: v => { const att = win.balanceMax - v
                                        if (ch.isLeft) device.balanceLeft = att; else device.balanceRight = att } } } }
                        Hint { text: "Double-click to reset"; color: Theme.n[500]; font.pixelSize: 11 }
                    }
                  } }

                // Filters ----------------------------------------------------
                ScrollView { id: filterScroll; clip: true; contentWidth: availableWidth
                    GridLayout { width: filterScroll.availableWidth
                        columns: Math.max(1, Math.floor((width + 12) / 242)); rowSpacing: 12; columnSpacing: 12
                        uniformCellWidths: true
                        Repeater { model: ["Sharp Roll-Off", "Slow Roll-Off", "Short Delay Sharp Roll-Off", "Short Delay Slow Roll-Off",
                                           "Apodizing Fast Roll-Off", "Low Dispersion Short Delay", "Brick Wall"]
                            FilterCard { Layout.fillWidth: true; text: modelData; curveIndex: index
                                         checked: device.filter === index; onClicked: device.filter = index } } }
                }
              }
              Text { anchors.centerIn: parent; visible: !device.connected; text: "Connect a FiiO KA3 over USB"
                     color: Theme.n[300]; font.pixelSize: 14 }
            }
            RowLayout { Layout.fillWidth: true
                Text { text: "Changes apply to the device immediately."; color: Theme.n[500]; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                Text { id: errorText; color: Theme.a[300]; font.pixelSize: 12 } }
        }
    }
}
