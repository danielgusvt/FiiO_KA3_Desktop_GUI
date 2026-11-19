# FiiO KA3 Control

A small Qt Quick (QML) desktop app for the FiiO KA3 USB DAC. It reads and sets volume, per-channel
balance, gain, impedance, hardware mute switch behaviour, dynamic power policy, indicator lights and
the lowpass filter, and shows the firmware version and the current sample rate.

The USB protocol was reverse-engineered from the FiiO Control Android app; see
[docs/FiiO_KA3_Reverse_Engineering.md](docs/FiiO_KA3_Reverse_Engineering.md).

## Requirements

- Qt 6.8 or newer with Quick, Quick Controls 2 and Svg (developed with 6.9.1)
- CMake 3.16+ and a C++ compiler
- libusb-1.0 (development files)

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.1/gcc_64
cmake --build build
```

In Qt Creator: open `CMakeLists.txt`, then *Projects → Build → Add → Release*, and turn
**QML debugging and profiling** off for that configuration.

## Install (self-contained, Linux)

The install step bundles Qt (libraries, QML modules, X11 and Wayland platform plugins, SVG plugin)
next to the binary, so the result runs without a Qt installation. Debug connectors are left out.

```sh
cmake --install build --prefix ~/.local/opt/ka3-control --strip
```

`packaging/install-local.sh <build-dir>` does the same and also adds an application-menu entry and icon.
libusb-1.0 is the only runtime dependency taken from the system.

## USB permissions

The app talks to the device through libusb and needs access to it without root. Install the udev rule,
then replug the DAC:

```sh
sudo cp packaging/69-fiio-ka3.rules /etc/udev/rules.d/
sudo udevadm control --reload && sudo udevadm trigger
```

## Layout

| Path | Purpose |
| --- | --- |
| `fiioka3.h` | USB protocol (libusb) |
| `devicebackend.*` | Main-thread facade over a USB worker thread; polls the device every 1.5 s |
| `Main.qml`, `*.qml` | UI |
| `docs/` | Protocol documentation |
| `packaging/` | udev rule, desktop entry, icon, local install script |

## Credits

Icons are [Phosphor Icons](https://phosphoricons.com) (MIT), inlined in `Icon.qml`.
