#ifndef FIIOKA3_H
#define FIIOKA3_H

#include <libusb-1.0/libusb.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <map>
#include <sstream>
#include <vector>

class FiiOKA3 {
public:
    static constexpr uint16_t VID = 0x2972;
    static constexpr uint16_t PID = 0x0053;

    // Request Constants
    static constexpr uint8_t REQ_WRITE_TYPE = 0x43;
    static constexpr uint8_t REQ_WRITE_CMD = 0xA0;

    static constexpr uint8_t REQ_READ_TYPE = 0xC3;
    static constexpr uint8_t REQ_READ_CMD = 0xA1;
    static constexpr uint16_t W_INDEX = 0x9A0;

    static constexpr uint8_t CMD_PREFIX[2] = {0xC7, 0xA5};

    // Command Bytes
    static constexpr uint8_t CMD_FILTER = 0x01;
    static constexpr uint8_t CMD_GAIN = 0x02;
    static constexpr uint8_t CMD_MAX_VOL = 0x03;
    static constexpr uint8_t CMD_VOL = 0x04;
    static constexpr uint8_t CMD_BALANCE = 0x05;
    static constexpr uint8_t CMD_IMPEDANCE = 0x06;
    static constexpr uint8_t CMD_INDICATOR = 0x07;
    static constexpr uint8_t CMD_MUTE = 0x08;
    static constexpr uint8_t CMD_POWER_SAVE = 0x09;

    // Query Request Headers
    static constexpr uint8_t QUERY_FIRMWARE  = 0xA0;
    static constexpr uint8_t QUERY_SAMPLE    = 0xA1;
    static constexpr uint8_t QUERY_AUDIO     = 0xA2;
    static constexpr uint8_t QUERY_SETTINGS  = 0xA3;

    FiiOKA3() {
        if (libusb_init(&ctx) < 0) {
            throw std::runtime_error("Failed to initialize libusb");
        }

        dev = libusb_open_device_with_vid_pid(ctx, VID, PID);
        if (!dev) {
            libusb_exit(ctx);
            throw std::runtime_error("FiiO KA3 not found. Check connections.");
        }
    }

    ~FiiOKA3() {
        if (dev) {
            libusb_close(dev);
        }
        libusb_exit(ctx);
        // std::cout << "closing gracefully" << std::endl;
    }


    // Device units 0..240 (0 = quietest). The wire value is the attenuation, 240 - level.
    void setVolume(int level) {
        sendCommand(CMD_VOL, 240 - clamp(level, 0, 240));
    }

    void setMaxVolume(uint8_t level) {
        sendCommand(CMD_MAX_VOL, clamp(level, 0, 120) * 2);
    }

    void setGain(bool high) {
        uint8_t val = high ? 2 : 1;
        sendCommand(CMD_GAIN, val);
    }

    void setImpedance(bool high) {
        uint8_t val = high ? 2 : 1;
        sendCommand(CMD_IMPEDANCE, val);
    }

    // Per-channel attenuation, 0..12 each (0 = none). The two values are independent.
    void setBalance(int left, int right) {
        sendCommand(CMD_BALANCE, clamp(right, 0, 12), clamp(left, 0, 12));
    }

    void setFilter(uint8_t index) {
        sendCommand(CMD_FILTER, 0x07, index);
    }

    void setIndicatorLight(uint8_t mode) {
        sendCommand(CMD_INDICATOR, mode);
    }

    void setHardwareMute(bool enable) {
        sendCommand(CMD_MUTE, enable ? 1 : 0);
    }

    void setPowerSave(bool enable) {
        sendCommand(CMD_POWER_SAVE, enable ? 1 : 0);
    }

    struct AudioStatus {
        int max_volume;      // device units 0..240
        int current_volume;  // device units 0..240
        int balance_left;   // attenuation 0..12
        int balance_right;  // attenuation 0..12
    };

    struct SettingsStatus {
        bool power_save;
        bool hardware_mute;
        int filter_index;
        bool gain_high;
        bool impedance_high;
        int indicator_mode;
    };

    std::string getFirmwareVersion() {
        std::vector<uint8_t> res = query(QUERY_FIRMWARE);
        if (res.empty()) throw std::runtime_error("Bad firmware response");
        std::ostringstream oss;
        oss << "V " << int(res[3]) << "." << int(res[4]) << "." << int(res[5]);
        return oss.str();
    }

    struct SampleRate {
        int hz;             // 0 if unknown
        std::string label;  // "44.1kHz", "DoP64", ...
    };

    SampleRate getSampleRateInfo() {
        std::vector<uint8_t> res = query(QUERY_SAMPLE);
        if (res.empty()) throw std::runtime_error("Bad sample rate response");

        struct Entry { int hz; const char *label; };
        static const std::map<uint8_t, Entry> rates = {
            {0x10, {44100, "44.1kHz"}}, {0x11, {48000, "48kHz"}}, {0x12, {88200, "88.2kHz"}},
            {0x13, {96000, "96kHz"}}, {0x14, {176400, "176.4kHz"}}, {0x15, {192000, "192kHz"}},
            {0x20, {176400, "DoP64"}}, {0x40, {2822400, "Native64"}}
        };

        auto it = rates.find(res[3]);
        if (it != rates.end()) {
            return {it->second.hz, it->second.label};
        }
        return {0, "Unknown (" + std::to_string(res[3]) + ")"};
    }

    std::string getSampleRate() {
        return getSampleRateInfo().label;
    }

    AudioStatus getAudioStatus() {
        std::vector<uint8_t> res = query(QUERY_AUDIO);
        if (res.empty()) throw std::runtime_error("Bad audio status response");
        AudioStatus s;
        s.max_volume = 240 - res[3];
        s.current_volume = 240 - res[4];
        s.balance_right = res[5];
        s.balance_left = res[6];
        return s;
    }

    SettingsStatus getSettingsStatus() {
        std::vector<uint8_t> res = query(QUERY_SETTINGS);
        if (res.empty()) throw std::runtime_error("Bad settings response");
        SettingsStatus s;

        s.power_save = (res[1] == 1);
        s.hardware_mute = (res[2] == 1);
        s.filter_index = res[3];
        s.gain_high = (res[4] == 2);
        s.impedance_high = (res[5] == 2);
        s.indicator_mode = res[6];
        return s;
    }


private:
    libusb_context *ctx = nullptr;
    libusb_device_handle *dev = nullptr;
    int interface_id = 1;
    bool kernel_driver_detached = false;

    int clamp(int n, int min, int max) {
        return n < min ? min : (n > max ? max : n);
    }

    void sendCommand(uint8_t cmd_byte, uint8_t v1 = 0, uint8_t v2 = 0) {
        uint8_t data[7] = {CMD_PREFIX[0], CMD_PREFIX[1], cmd_byte, v1, v2, 0x00, 0x00};
        int transferred = libusb_control_transfer(
            dev,
            REQ_WRITE_TYPE,
            REQ_WRITE_CMD,
            interface_id,
            W_INDEX,
            data,
            sizeof(data),
            1000
            );

        if (transferred < 0) {
            throw std::runtime_error("Failed to send command");
        }
    }

    std::vector<uint8_t> query(uint8_t queryByte) {
        sendCommand(queryByte);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        unsigned char data[7];
        int transferred = libusb_control_transfer(dev, REQ_READ_TYPE, REQ_READ_CMD, 0, W_INDEX, data, 7, 1000);
        if (transferred < 0) throw std::runtime_error("Failed to read response");
        if (transferred < 7 || data[0] != 0xA7) return {};
        return std::vector<uint8_t>(data, data + 7);
    }
};

#endif // FIIOKA3_H
