#include "devicebackend.h"
#include <QDateTime>
#include <algorithm>
#include <iostream>

namespace {
constexpr int PollIntervalMs = 1500;
// A poll that was in flight while the user changed something carries the old value.
constexpr qint64 WriteSettleMs = 800;
}

// ---- worker (USB thread) ----------------------------------------------------

void Ka3Worker::start()
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this] { poll(); });
    m_timer->start(PollIntervalMs);
    poll(true);
}

void Ka3Worker::disconnectDevice()
{
    if (!m_ka3) return;
    m_ka3.reset();
    emit connectionChanged(false, QString());
}

void Ka3Worker::poll(bool force)
{
    try {
        if (!m_ka3) {
            auto ka3 = std::make_unique<FiiOKA3>();
            const QString fw = QString::fromStdString(ka3->getFirmwareVersion());
            m_ka3 = std::move(ka3);
            emit connectionChanged(true, fw);
            force = true;
        }
        const auto audio = m_ka3->getAudioStatus();
        const auto set = m_ka3->getSettingsStatus();
        const auto rate = m_ka3->getSampleRateInfo();

        Ka3State s;
        s.volume = audio.current_volume;
        s.balanceLeft = audio.balance_left;
        s.balanceRight = audio.balance_right;
        s.filter = set.filter_index;
        s.gain = set.gain_high ? 1 : 0;
        s.impedance = set.impedance_high ? 1 : 0;
        s.indicator = set.indicator_mode;
        s.hardwareMute = set.hardware_mute;
        s.dynamicPower = set.power_save;
        s.sampleRate = rate.hz;
        s.sampleRateLabel = QString::fromStdString(rate.label);
        emit stateRead(s, force);
    } catch (const std::exception &e) {
        // Not connected yet (normal while unplugged) or the device went away.
        if (m_ka3) std::cerr << "KA3 poll failed: " << e.what() << std::endl;
        disconnectDevice();
    }
}

void Ka3Worker::write(int prop, int value)
{
    if (!m_ka3) {
        emit writeFailed(tr("Device not connected"));
        return;
    }
    try {
        switch (prop) {
        case Volume:       m_ka3->setVolume(value); break;
        case Balance:      m_ka3->setBalance(value & 0xFF, value >> 8); break; // left | right << 8
        case Filter:       m_ka3->setFilter(static_cast<uint8_t>(value)); break;
        case Gain:         m_ka3->setGain(value != 0); break;
        case Impedance:    m_ka3->setImpedance(value != 0); break;
        case Indicator:    m_ka3->setIndicatorLight(static_cast<uint8_t>(value)); break;
        case HardwareMute: m_ka3->setHardwareMute(value != 0); break;
        case DynamicPower: m_ka3->setPowerSave(value != 0); break;
        }
    } catch (const std::exception &e) {
        std::cerr << "KA3 write failed: " << e.what() << std::endl;
        emit writeFailed(tr("Couldn't write to the device"));
        poll(true); // re-read so the UI reverts to what the device really has
    }
}

// ---- backend (main thread) --------------------------------------------------

DeviceBackend::DeviceBackend(QObject *parent)
    : QObject(parent), m_worker(new Ka3Worker)
{
    qRegisterMetaType<Ka3State>();
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(&m_thread, &QThread::started, m_worker, &Ka3Worker::start);
    connect(m_worker, &Ka3Worker::connectionChanged, this, &DeviceBackend::onConnection);
    connect(m_worker, &Ka3Worker::stateRead, this, &DeviceBackend::onState);
    connect(m_worker, &Ka3Worker::writeFailed, this, &DeviceBackend::writeFailed);
    connect(this, &DeviceBackend::requestWrite, m_worker, &Ka3Worker::write);
    m_thread.start();
}

DeviceBackend::~DeviceBackend()
{
    m_thread.quit();
    m_thread.wait();
}

void DeviceBackend::onConnection(bool connected, const QString &firmware)
{
    if (m_firmware != firmware) {
        m_firmware = firmware;
        emit firmwareChanged();
    }
    if (m_connected != connected) {
        m_connected = connected;
        emit connectedChanged();
    }
}

bool DeviceBackend::recentlyWritten(Ka3Worker::Prop prop) const
{
    return QDateTime::currentMSecsSinceEpoch() - m_writtenAt[prop] < WriteSettleMs;
}

void DeviceBackend::onState(const Ka3State &n, bool force)
{
    // Assigns and emits only on change, unless a local write is still settling.
    auto apply = [&](Ka3Worker::Prop prop, auto &cur, const auto &val, auto signal) {
        if (cur == val || (!force && recentlyWritten(prop))) return;
        cur = val;
        emit (this->*signal)();
    };
    apply(Ka3Worker::Volume, m_s.volume, n.volume, &DeviceBackend::volumeChanged);
    apply(Ka3Worker::Balance, m_s.balanceLeft, n.balanceLeft, &DeviceBackend::balanceLeftChanged);
    apply(Ka3Worker::Balance, m_s.balanceRight, n.balanceRight, &DeviceBackend::balanceRightChanged);
    apply(Ka3Worker::Filter, m_s.filter, n.filter, &DeviceBackend::filterChanged);
    apply(Ka3Worker::Gain, m_s.gain, n.gain, &DeviceBackend::gainChanged);
    apply(Ka3Worker::Impedance, m_s.impedance, n.impedance, &DeviceBackend::impedanceChanged);
    apply(Ka3Worker::Indicator, m_s.indicator, n.indicator, &DeviceBackend::indicatorChanged);
    apply(Ka3Worker::HardwareMute, m_s.hardwareMute, n.hardwareMute, &DeviceBackend::hardwareMuteChanged);
    apply(Ka3Worker::DynamicPower, m_s.dynamicPower, n.dynamicPower, &DeviceBackend::dynamicPowerChanged);
    if (m_s.sampleRate != n.sampleRate || m_s.sampleRateLabel != n.sampleRateLabel) {
        m_s.sampleRate = n.sampleRate;
        m_s.sampleRateLabel = n.sampleRateLabel;
        emit sampleRateChanged();
    }
}

void DeviceBackend::write(Ka3Worker::Prop prop, int value)
{
    m_writtenAt[prop] = QDateTime::currentMSecsSinceEpoch();
    emit requestWrite(prop, value);
}

void DeviceBackend::setHardwareMute(bool on)
{
    if (m_s.hardwareMute == on) return;
    m_s.hardwareMute = on; emit hardwareMuteChanged(); write(Ka3Worker::HardwareMute, on);
}

void DeviceBackend::setDynamicPower(bool on)
{
    if (m_s.dynamicPower == on) return;
    m_s.dynamicPower = on; emit dynamicPowerChanged(); write(Ka3Worker::DynamicPower, on);
}

void DeviceBackend::setIndicator(int mode)
{
    mode = std::clamp(mode, 0, 2);
    if (m_s.indicator == mode) return;
    m_s.indicator = mode; emit indicatorChanged(); write(Ka3Worker::Indicator, mode);
}

void DeviceBackend::setImpedance(int v)
{
    v = v ? 1 : 0;
    if (m_s.impedance == v) return;
    m_s.impedance = v; emit impedanceChanged(); write(Ka3Worker::Impedance, v);
}

void DeviceBackend::setGain(int v)
{
    v = v ? 1 : 0;
    if (m_s.gain == v) return;
    m_s.gain = v; emit gainChanged(); write(Ka3Worker::Gain, v);
}

void DeviceBackend::setVolume(int deviceUnits)
{
    deviceUnits = std::clamp(deviceUnits, 0, 240);
    if (m_s.volume == deviceUnits) return;
    m_s.volume = deviceUnits; emit volumeChanged(); write(Ka3Worker::Volume, deviceUnits);
}

// Both channels go out in one command, so each setter sends the pair.
void DeviceBackend::setBalanceLeft(int attenuation)
{
    attenuation = std::clamp(attenuation, 0, BalanceRange);
    if (m_s.balanceLeft == attenuation) return;
    m_s.balanceLeft = attenuation; emit balanceLeftChanged();
    write(Ka3Worker::Balance, m_s.balanceLeft | m_s.balanceRight << 8);
}

void DeviceBackend::setBalanceRight(int attenuation)
{
    attenuation = std::clamp(attenuation, 0, BalanceRange);
    if (m_s.balanceRight == attenuation) return;
    m_s.balanceRight = attenuation; emit balanceRightChanged();
    write(Ka3Worker::Balance, m_s.balanceLeft | m_s.balanceRight << 8);
}

void DeviceBackend::setFilter(int index)
{
    index = std::clamp(index, 0, 6);
    if (m_s.filter == index) return;
    m_s.filter = index; emit filterChanged(); write(Ka3Worker::Filter, index);
}
