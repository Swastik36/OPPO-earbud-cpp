#include "EarbudController.hpp"
#include <QtCore/QMetaObject>

EarbudController::EarbudController(QObject* parent) : QObject(parent) {
    m_device.register_push_callback([this](const oppo::OppoFrame& f, const oppo::OppoMessage& m) {
        QMetaObject::invokeMethod(this, [this, f, m]() {
            this->handlePushNotification(f, m);
        }, Qt::QueuedConnection);
    });
}

EarbudController::~EarbudController() {
    disconnectDevice();
}

void EarbudController::connectDevice(const QString& mac) {
    std::string std_mac = mac.toStdString();

    m_statusText = "Connecting to " + mac + "...";
    emit statusTextChanged();

    QtConcurrent::run([this, std_mac]() {
        QMutexLocker locker(&m_deviceMutex);
        auto res = m_device.connect(std_mac, 15);

        QMetaObject::invokeMethod(this, [this, res, std_mac]() {
            m_connected = res.has_value() && res.value();
            emit connectedChanged();

            if (m_connected) {
                m_statusText = "Connected";
                emit statusTextChanged();
                fetchInitialState();
            } else {
                m_statusText = "Connection Failed (" + QString::fromStdString(oppo::error_to_string(res.error())) + ")";
                emit statusTextChanged();
            }
        });
    });
}

void EarbudController::disconnectDevice() {
    QtConcurrent::run([this]() {
        QMutexLocker locker(&m_deviceMutex);
        m_device.disconnect();

        QMetaObject::invokeMethod(this, [this]() {
            m_connected = false;
            m_statusText = "Disconnected";
            emit connectedChanged();
            emit statusTextChanged();
        });
    });
}

void EarbudController::fetchInitialState() {
    QtConcurrent::run([this]() {
        QMutexLocker locker(&m_deviceMutex);
        auto b_res = m_device.get_battery();
        auto eq_res = m_device.get_eq();
        auto gm_res = m_device.get_game_mode();

        QMetaObject::invokeMethod(this, [this, b_res, eq_res, gm_res]() {
            if (b_res.has_value()) {
                const auto& b = b_res.value();
                m_leftBattery = b.left.percentage;
                m_leftCharging = b.left.is_charging;
                m_rightBattery = b.right.percentage;
                m_rightCharging = b.right.is_charging;
                m_caseBattery = b.case_val.percentage;
                m_caseCharging = b.case_val.is_charging;
                emit batteryChanged();
            }
            if (eq_res.has_value()) {
                m_eq = static_cast<int>(eq_res.value());
                emit eqChanged();
            }
            if (gm_res.has_value()) {
                m_gameMode = gm_res.value();
                emit gameModeChanged();
            }
        });
    });
}

void EarbudController::setEQ(int presetIndex) {
    auto preset = static_cast<oppo::EQPreset>(presetIndex);
    QtConcurrent::run([this, preset]() {
        QMutexLocker locker(&m_deviceMutex);
        if (m_device.set_eq(preset).has_value()) {
            QMetaObject::invokeMethod(this, [this, preset]() {
                m_eq = static_cast<int>(preset);
                emit eqChanged();
            });
        }
    });
}

void EarbudController::setGameMode(bool enable) {
    QtConcurrent::run([this, enable]() {
        QMutexLocker locker(&m_deviceMutex);
        if (m_device.set_game_mode(enable).has_value()) {
            QMetaObject::invokeMethod(this, [this, enable]() {
                m_gameMode = enable;
                emit gameModeChanged();
            });
        }
    });
}

void EarbudController::handlePushNotification(const oppo::OppoFrame&, const oppo::OppoMessage& msg) {
    if (std::holds_alternative<oppo::EarbudsBattery>(msg)) {
        const auto& b = std::get<oppo::EarbudsBattery>(msg);
        m_leftBattery = b.left.percentage;
        m_leftCharging = b.left.is_charging;
        m_rightBattery = b.right.percentage;
        m_rightCharging = b.right.is_charging;
        m_caseBattery = b.case_val.percentage;
        m_caseCharging = b.case_val.is_charging;
        emit batteryChanged();
    }
}
