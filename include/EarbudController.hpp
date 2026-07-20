#ifndef EARBUD_CONTROLLER_HPP
#define EARBUD_CONTROLLER_HPP

#include <QObject>
#include <QString>
#include <QMutex>
#include <QtConcurrent>
#include "oppo/device.hpp"

class EarbudController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(int leftBattery READ leftBattery NOTIFY batteryChanged)
    Q_PROPERTY(bool leftCharging READ leftCharging NOTIFY batteryChanged)
    Q_PROPERTY(int rightBattery READ rightBattery NOTIFY batteryChanged)
    Q_PROPERTY(bool rightCharging READ rightCharging NOTIFY batteryChanged)
    Q_PROPERTY(int caseBattery READ caseBattery NOTIFY batteryChanged)
    Q_PROPERTY(bool caseCharging READ caseCharging NOTIFY batteryChanged)
    Q_PROPERTY(int currentEQ READ currentEQ NOTIFY eqChanged)
    Q_PROPERTY(bool gameMode READ gameMode NOTIFY gameModeChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit EarbudController(QObject* parent = nullptr);
    ~EarbudController() override;

    bool isConnected() const { return m_connected; }
    int leftBattery() const { return m_leftBattery; }
    bool leftCharging() const { return m_leftCharging; }
    int rightBattery() const { return m_rightBattery; }
    bool rightCharging() const { return m_rightCharging; }
    int caseBattery() const { return m_caseBattery; }
    bool caseCharging() const { return m_caseCharging; }
    int currentEQ() const { return m_eq; }
    bool gameMode() const { return m_gameMode; }
    QString statusText() const { return m_statusText; }

    Q_INVOKABLE void connectDevice(const QString& mac = "60:55:56:22:49:A0");
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE void setEQ(int presetIndex); // 0=Original, 1=Vocals, 2=Bass
    Q_INVOKABLE void setGameMode(bool enable);

signals:
    void connectedChanged();
    void batteryChanged();
    void eqChanged();
    void gameModeChanged();
    void statusTextChanged();

private:
    void fetchInitialState();
    void handlePushNotification(const oppo::OppoFrame& frame, const oppo::OppoMessage& msg);

    oppo::OppoDevice m_device;
    QMutex m_deviceMutex;
    bool m_connected{false};
    int m_leftBattery{0}, m_rightBattery{0}, m_caseBattery{0};
    bool m_leftCharging{false}, m_rightCharging{false}, m_caseCharging{false};
    int m_eq{0};
    bool m_gameMode{false};
    QString m_statusText{"Disconnected"};
};

#endif // EARBUD_CONTROLLER_HPP
