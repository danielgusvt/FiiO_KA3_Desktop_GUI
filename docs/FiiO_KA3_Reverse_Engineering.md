# FiiO KA3 USB DAC Reverse Engineering

This document summarizes the reverse-engineered control protocol for the FiiO KA3 USB DAC, derived from the FiiO Control Android application (v4.0.3).

## Device Identification
- **idVendor**: `0x2972`
- **idProduct**: `0x0053`

## Communication Layer (USB Protocol)
Control of the KA3 is performed via **USB Control Transfers** (Vendor Specific).

### Write Operations (Send Commands)
- **bmRequestType**: `0x43` (Vendor, Out, Other)
- **bRequest**: `0xA0`
- **wValue**: `0x0000` (Interface ID, typically 0 or 1)
- **wIndex**: `0x09A0` (2464)
- **Data Length**: 7 bytes

### Read Operations (Queries)
- **bmRequestType**: `0xC3` (Vendor, In, Other)
- **bRequest**: `0xA1`
- **wValue**: `0x0000`
- **wIndex**: `0x09A0`
- **Data Length**: 7 bytes

---

## Command Protocol (Write)
All commands follow a 7-byte buffer structure starting with a 2-byte prefix.

**Format**: `[0xC7, 0xA5, <Command_Byte>, <Value_1>, <Value_2>, 0x00, 0x00]`

| Feature | Command Byte | Value Logic |
| :--- | :--- | :--- |
| **Filter** | `0x01` | `Value1=0x07`, `Value2=index(1-7)` |
| **Gain** | `0x02` | `Value1`: `1` (Low), `2` (High) |
| **Max Volume** | `0x03` | `Value1`: `0-240` (maps to 0-120 in UI) |
| **Current Volume**| `0x04` | `Value1`: `0-240` (maps to 0-120 in UI) |
| **Channel Balance**| `0x05` | `Value1`: Right Bias (`0-12`), `Value2`: Left Bias (`0-12`) |
| **Impedance** | `0x06` | `Value1`: `1` (Low), `2` (High) |
| **Indicator Light**| `0x07` | `Value1`: `0` (On), `1` (Once Off), `2` (Always Off) |
| **Hardware Mute** | `0x08` | `Value1`: `0` (Off), `1` (On) |
| **Power Save** | `0x09` | `Value1`: `0` (Off), `1` (On) |

### Filter Indices
1. Sharp Roll-Off
2. Slow Roll-Off
3. Short Delay Sharp Roll-Off
4. Short Delay Slow Roll-Off
5. Apodizing fast roll-off
6. Low Dispersion Short Delay
7. Brick Wall

---

## Query Protocol (Read)
To read device state, send the **Query Header** byte as a command, then perform a control read. Responses always start with `0xA7`.

### Query Headers
- **Firmware Version**: `0xA0`
- **Sample Rate**: `0xA1`
- **Audio Status**: `0xA2` (Max Vol, Vol, Balance)
- **Hardware Settings**: `0xA3` (Filter, Gain, Mute, Power Save)

### Sample Rate Mapping (Byte 3)
| Hex | Rate | Hex | Rate |
| :--- | :--- | :--- | :--- |
| `0x10` | 44.1kHz | `0x11` | 48kHz |
| `0x12` | 88.2kHz | `0x13` | 96kHz |
| `0x14` | 176.4kHz | `0x15` | 192kHz |
| `0x20` | DoP64 | `0x40` | Native64 |

---

## Smali Source References
The following files contain the primary logic and constant values:

1.  **Low-Level Transfer**: `smali_classes2\com\google\android\gms\internal\mlkit_vision_barcode\ob.smali`
    - Contains the `controlTransfer` calls and method wrappers for read (`r`) and write (`B`).
2.  **Command Headers**: `smali_classes2\com\google\android\gms\internal\mlkit_vision_barcode\lb.smali`
    - Defines the `0xC7 A5 XX` byte arrays used as prefixes.
3.  **UI Mapping**: `smali\a6\a.smali`
    - Method `I(IFI)V` handles the conversion from UI slider floats to hardware bytes (e.g., UI `0-120` to Hardware `0-240`).
4.  **State Models**: `smali
c\b.smali` (Audio) and `smali
c\d.smali` (Settings)
    - Define the data structures and response parsing logic.
