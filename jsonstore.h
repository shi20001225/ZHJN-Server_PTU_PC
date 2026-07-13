#ifndef JSONSTORE_H
#define JSONSTORE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDateTime>
#include <QMap>
#include <QDebug>
#include "pack.h"

struct DeviceRecord {
    QString deviceNumber;   // 设备号 (BCD)
    QString cardNumber;     // 卡号
    quint8  server;         // 服务器
    quint8  deviceId;       // 设备ID
    quint8  funcCode;       // 功能码
    quint8  flag;           // 标志
    quint16 extend;         // 拓展

    // 电力数据
    double voltage;
    double current;
    double power;
    double energy;
    double powerFactor;
    double co2;
    double temperature;
    double frequency;

    // 元数据
    QDateTime lastUpdate;   // 最后更新时间
    int updateCount;        // 更新次数

    // ========== 储能柜系统专用字段 ==========
    quint8 powerSupplyStatus;   // 系统供电状态 (1:国电, 2:风光, 3:电池)

    // 国电充电数据
    double gridChargeVoltage;
    double gridChargeCurrent;
    double gridChargePower;
    double gridChargeEnergy;

    // 电池数据
    quint16 batteryLevel;
    double batteryVoltage;
    double batteryChargeCurrent;
    double batteryDischargeCurrent;
    double batteryTemperature;

    // ========== 储能柜系统：非国电供电期间的起始记录 ==========
    bool isInSavingMode;        // 是否处于节约模式（非国电供电中）
    double savingStartEnergy;   // 开始节约时的逆变器电能读数
    QDateTime savingStartTime;  // 开始节约的时间

    // 转成 JSON
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["deviceNumber"] = deviceNumber;
        obj["cardNumber"] = cardNumber;
        obj["server"] = QString("0x%1").arg(server, 2, 16, QChar('0'));
        obj["deviceId"] = QString("0x%1").arg(deviceId, 2, 16, QChar('0'));
        obj["funcCode"] = QString("0x%1").arg(funcCode, 2, 16, QChar('0'));
        obj["flag"] = flag;
        obj["extend"] = QString("0x%1").arg(extend, 4, 16, QChar('0'));

        obj["voltage"] = voltage;
        obj["current"] = current;
        obj["power"] = power;
        obj["energy"] = energy;
        obj["powerFactor"] = powerFactor;
        obj["co2"] = co2;
        obj["temperature"] = temperature;
        obj["frequency"] = frequency;

        // 储能柜字段
        obj["powerSupplyStatus"] = powerSupplyStatus;
        obj["gridChargeVoltage"] = gridChargeVoltage;
        obj["gridChargeCurrent"] = gridChargeCurrent;
        obj["gridChargePower"] = gridChargePower;
        obj["gridChargeEnergy"] = gridChargeEnergy;
        obj["batteryLevel"] = batteryLevel;
        obj["batteryVoltage"] = batteryVoltage;
        obj["batteryChargeCurrent"] = batteryChargeCurrent;
        obj["batteryDischargeCurrent"] = batteryDischargeCurrent;
        obj["batteryTemperature"] = batteryTemperature;

        // 节约模式记录
        obj["isInSavingMode"] = isInSavingMode;
        obj["savingStartEnergy"] = savingStartEnergy;
        obj["savingStartTime"] = savingStartTime.isValid() ? savingStartTime.toString("yyyy-MM-dd hh:mm:ss") : "";

        obj["lastUpdate"] = lastUpdate.toString("yyyy-MM-dd hh:mm:ss");
        obj["updateCount"] = updateCount;

        return obj;
    }

    // 从 JSON 解析
    static DeviceRecord fromJson(const QJsonObject &obj) {
        DeviceRecord rec;
        rec.deviceNumber = obj["deviceNumber"].toString();
        rec.cardNumber = obj["cardNumber"].toString();
        rec.server = static_cast<quint8>(obj["server"].toString().toUInt(nullptr, 16));
        rec.deviceId = static_cast<quint8>(obj["deviceId"].toString().toUInt(nullptr, 16));
        rec.funcCode = static_cast<quint8>(obj["funcCode"].toString().toUInt(nullptr, 16));
        rec.flag = static_cast<quint8>(obj["flag"].toInt());
        rec.extend = static_cast<quint16>(obj["extend"].toString().toUInt(nullptr, 16));

        rec.voltage = obj["voltage"].toDouble();
        rec.current = obj["current"].toDouble();
        rec.power = obj["power"].toDouble();
        rec.energy = obj["energy"].toDouble();
        rec.powerFactor = obj["powerFactor"].toDouble();
        rec.co2 = obj["co2"].toDouble();
        rec.temperature = obj["temperature"].toDouble();
        rec.frequency = obj["frequency"].toDouble();

        rec.powerSupplyStatus = static_cast<quint8>(obj["powerSupplyStatus"].toInt());
        rec.gridChargeVoltage = obj["gridChargeVoltage"].toDouble();
        rec.gridChargeCurrent = obj["gridChargeCurrent"].toDouble();
        rec.gridChargePower = obj["gridChargePower"].toDouble();
        rec.gridChargeEnergy = obj["gridChargeEnergy"].toDouble();
        rec.batteryLevel = static_cast<quint16>(obj["batteryLevel"].toInt());
        rec.batteryVoltage = obj["batteryVoltage"].toDouble();
        rec.batteryChargeCurrent = obj["batteryChargeCurrent"].toDouble();
        rec.batteryDischargeCurrent = obj["batteryDischargeCurrent"].toDouble();
        rec.batteryTemperature = obj["batteryTemperature"].toDouble();

        // 节约模式记录（兼容旧数据）
        rec.isInSavingMode = obj["isInSavingMode"].toBool();
        rec.savingStartEnergy = obj["savingStartEnergy"].toDouble();
        QString savingStartTimeStr = obj["savingStartTime"].toString();
        if (!savingStartTimeStr.isEmpty()) {
            rec.savingStartTime = QDateTime::fromString(savingStartTimeStr, "yyyy-MM-dd hh:mm:ss");
        }

        rec.lastUpdate = QDateTime::fromString(obj["lastUpdate"].toString(), "yyyy-MM-dd hh:mm:ss");
        rec.updateCount = obj["updateCount"].toInt();

        return rec;
    }
};


class JsonStore : public QObject
{
    Q_OBJECT
public:
    explicit JsonStore(const QString &filePath = "device_data.json", QObject *parent = nullptr);
    ~JsonStore();

    // 添加或更新设备数据
    void updateDevice(const PowerData &record);

    // 更新储能柜系统的节约模式状态
    void updateSavingState(const QString &deviceNumber, bool isInSavingMode, double savingStartEnergy, const QDateTime &savingStartTime);

    // 根据设备号获取数据
    DeviceRecord getDeviceByNumber(const QString &deviceNumber) const;

    // 根据卡号获取数据
    DeviceRecord getDeviceByCard(const QString &cardNumber) const;

    // 获取所有设备
    QMap<QString, DeviceRecord> getAllDevices() const;

    // 删除设备
    void removeDevice(const QString &deviceNumber);

    // 清空所有数据
    void clearAll();

    // 保存到文件
    bool saveToFile();

    // 从文件加载
    bool loadFromFile();

    // 获取设备数量
    int deviceCount() const;

signals:
    void deviceUpdated(const QString &deviceNumber);
    void dataSaved(bool success);

private:
    QString m_filePath;
    QMap<QString, DeviceRecord> m_devices;  // key = deviceNumber
};

#endif // JSONSTORE_H
