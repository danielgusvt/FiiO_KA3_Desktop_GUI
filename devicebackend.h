#ifndef DEVICEBACKEND_H
#define DEVICEBACKEND_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QTimer>
#include <array>
#include <memory>
#include "fiioka3.h"

// Everything the device reports, in the units of the backend contract.
struct Ka3State {
    int volume = 0;          // device units 0..240
    int balanceLeft = 0;     // attenuation 0..12 (0 = none)
    int balanceRight = 0;
    int filter = 0;          // 0..6
    int gain = 0;            // 0 low, 1 high
    int impedance = 0;       // 0 low, 1 high
    int indicator = 0;       // 0 on, 1 off until restart, 2 off always
    bool hardwareMute = false;
    bool dynamicPower = false;
    int sampleRate = 0;      // Hz, 0 unknown
    QString sampleRateLabel; // as reported, e.g. "44.1kHz" or "DoP64"
};
Q_DECLARE_METATYPE(Ka3State)

// Lives on its own thread: every libusb call blocks (queries sleep 100 ms).
class Ka3Worker : public QObject {
    Q_OBJECT
public:
    enum Prop { Volume, Balance, Filter, Gain, Impedance, Indicator, HardwareMute, DynamicPower, PropCount };
public slots:
    void start();
    void poll(bool force = false);
    void write(int prop, int value);
signals:
    void connectionChanged(bool connected, const QString &firmware);
    void stateRead(const Ka3State &state, bool force);
    void writeFailed(const QString &what);
private:
    void disconnectDevice();
    std::unique_ptr<FiiOKA3> m_ka3;
    QTimer *m_timer = nullptr;
};

// Main-thread facade following the README "backend contract". Setters write to the
// device and emit the *Changed signal; changes made on the device (hardware buttons)
// arrive through the periodic poll and emit the same signals.
class DeviceBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString firmware READ firmware NOTIFY firmwareChanged)
    Q_PROPERTY(int sampleRate READ sampleRate NOTIFY sampleRateChanged)
    Q_PROPERTY(QString sampleRateLabel READ sampleRateLabel NOTIFY sampleRateChanged)
    Q_PROPERTY(bool hardwareMute READ hardwareMute WRITE setHardwareMute NOTIFY hardwareMuteChanged)
    Q_PROPERTY(bool dynamicPower READ dynamicPower WRITE setDynamicPower NOTIFY dynamicPowerChanged)
    Q_PROPERTY(int indicator READ indicator WRITE setIndicator NOTIFY indicatorChanged)
    Q_PROPERTY(int impedance READ impedance WRITE setImpedance NOTIFY impedanceChanged)
    Q_PROPERTY(int gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int balanceLeft READ balanceLeft WRITE setBalanceLeft NOTIFY balanceLeftChanged)
    Q_PROPERTY(int balanceRight READ balanceRight WRITE setBalanceRight NOTIFY balanceRightChanged)
    Q_PROPERTY(int filter READ filter WRITE setFilter NOTIFY filterChanged)
public:
    static constexpr int BalanceRange = 12;

    explicit DeviceBackend(QObject *parent = nullptr);
    ~DeviceBackend() override;

    bool connected() const { return m_connected; }
    QString firmware() const { return m_firmware; }
    int sampleRate() const { return m_s.sampleRate; }
    QString sampleRateLabel() const { return m_s.sampleRateLabel; }
    bool hardwareMute() const { return m_s.hardwareMute; }
    bool dynamicPower() const { return m_s.dynamicPower; }
    int indicator() const { return m_s.indicator; }
    int impedance() const { return m_s.impedance; }
    int gain() const { return m_s.gain; }
    int volume() const { return m_s.volume; }
    int balanceLeft() const { return m_s.balanceLeft; }
    int balanceRight() const { return m_s.balanceRight; }
    int filter() const { return m_s.filter; }

public slots:
    void setHardwareMute(bool on);
    void setDynamicPower(bool on);
    void setIndicator(int mode);
    void setImpedance(int v);
    void setGain(int v);
    void setVolume(int deviceUnits);
    void setBalanceLeft(int attenuation);
    void setBalanceRight(int attenuation);
    void setFilter(int index);

signals:
    void connectedChanged();
    void firmwareChanged();
    void sampleRateChanged();
    void hardwareMuteChanged();
    void dynamicPowerChanged();
    void indicatorChanged();
    void impedanceChanged();
    void gainChanged();
    void volumeChanged();
    void balanceLeftChanged();
    void balanceRightChanged();
    void filterChanged();
    void writeFailed(const QString &what);

    // internal: to the worker thread
    void requestWrite(int prop, int value);

private slots:
    void onConnection(bool connected, const QString &firmware);
    void onState(const Ka3State &state, bool force);

private:
    void write(Ka3Worker::Prop prop, int value);
    bool recentlyWritten(Ka3Worker::Prop prop) const;

    QThread m_thread;
    Ka3Worker *m_worker;
    bool m_connected = false;
    QString m_firmware;
    Ka3State m_s;
    std::array<qint64, Ka3Worker::PropCount> m_writtenAt{};
};

#endif // DEVICEBACKEND_H
