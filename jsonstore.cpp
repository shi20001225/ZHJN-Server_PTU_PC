#include "jsonstore.h"

JsonStore::JsonStore(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_filePath(filePath)
{
    loadFromFile();
}

JsonStore::~JsonStore()
{
    saveToFile();
}

void JsonStore::appendIpToDevice(QString ip, QString deviceNumber)
{
    m_ipToDevice.insert(ip,deviceNumber);
}

void JsonStore::updateDevice(const PowerData &record)
{
    QString key = record.deviceNumber;

    if (m_devices.contains(key)) {
        // 更新已有设备
        DeviceRecord &existing = m_devices[key];

        // 保留储能柜系统的节约模式状态（在 calculate_CNG 中设置）
        bool oldIsInSavingMode = existing.isInSavingMode;
        double oldSavingStartEnergy = existing.savingStartEnergy;
        QDateTime oldSavingStartTime = existing.savingStartTime;

        existing.clientIp = record.ip;
        existing.server = record.server;
        existing.deviceId = record.deviceId;
        existing.funcCode = record.funcCode;
        existing.flag = record.flag;
        existing.extend = record.extend;
        existing.voltage = record.voltage;
        existing.current = record.current;
        existing.power = record.power;
        existing.energy = record.energy;
        existing.powerFactor = record.powerFactor;
        existing.co2 = record.co2;
        existing.temperature = record.temperature;
        existing.frequency = record.frequency;
        existing.lastUpdate = QDateTime::currentDateTime();
        existing.updateCount++;

        // 更新储能柜字段
        existing.powerSupplyStatus = record.powerSupplyStatus;
        existing.gridChargeVoltage = record.gridChargeVoltage;
        existing.gridChargeCurrent = record.gridChargeCurrent;
        existing.gridChargePower = record.gridChargePower;
        existing.gridChargeEnergy = record.gridChargeEnergy;
        existing.batteryLevel = record.batteryLevel;
        existing.batteryVoltage = record.batteryVoltage;
        existing.batteryChargeCurrent = record.batteryChargeCurrent;
        existing.batteryDischargeCurrent = record.batteryDischargeCurrent;
        existing.batteryTemperature = record.batteryTemperature;

        // 恢复节约模式状态（不能被 updateDevice 覆盖）
        existing.isInSavingMode = oldIsInSavingMode;
        existing.savingStartEnergy = oldSavingStartEnergy;
        existing.savingStartTime = oldSavingStartTime;

        qInfo() << "[更新设备] "<< key
                << "更新次数:"  << existing.updateCount;
    } else {
        // 新增设备
        DeviceRecord newRecord;

        newRecord.deviceNumber = record.deviceNumber;
        newRecord.cardNumber = record.cardNumber;
        newRecord.clientIp = record.ip;
        newRecord.server = record.server;
        newRecord.deviceId = record.deviceId;
        newRecord.funcCode = record.funcCode;
        newRecord.flag = record.flag;
        newRecord.extend = record.extend;
        newRecord.voltage = record.voltage;
        newRecord.current = record.current;
        newRecord.power = record.power;
        newRecord.energy = record.energy;
        newRecord.powerFactor = record.powerFactor;
        newRecord.co2 = record.co2;
        newRecord.temperature = record.temperature;
        newRecord.frequency = record.frequency ;
        newRecord.lastUpdate = QDateTime::currentDateTime();
        newRecord.updateCount = 1;

        // 储能柜字段
        newRecord.powerSupplyStatus = record.powerSupplyStatus;
        newRecord.gridChargeVoltage = record.gridChargeVoltage;
        newRecord.gridChargeCurrent = record.gridChargeCurrent;
        newRecord.gridChargePower = record.gridChargePower;
        newRecord.gridChargeEnergy = record.gridChargeEnergy;
        newRecord.batteryLevel = record.batteryLevel;
        newRecord.batteryVoltage = record.batteryVoltage;
        newRecord.batteryChargeCurrent = record.batteryChargeCurrent;
        newRecord.batteryDischargeCurrent = record.batteryDischargeCurrent;
        newRecord.batteryTemperature = record.batteryTemperature;

        // 节约模式初始化
        newRecord.isInSavingMode = false;
        newRecord.savingStartEnergy = 0.0;

        m_devices.insert(key, newRecord);

        qInfo() << "[新增设备]" << key;
    }

    emit deviceUpdated(key);
    //saveToFile();  // 实时保存



}

// 更新储能柜系统的节约模式状态（由 calculate_CNG 调用后同步）
void JsonStore::updateSavingState(const QString &deviceNumber, bool isInSavingMode, double savingStartEnergy, const QDateTime &savingStartTime)
{
    if (m_devices.contains(deviceNumber)) {
        DeviceRecord &existing = m_devices[deviceNumber];
        existing.isInSavingMode = isInSavingMode;
        existing.savingStartEnergy = savingStartEnergy;
        existing.savingStartTime = savingStartTime;
        qDebug() << "更新节约状态:" << deviceNumber
                 << "节约中:" << isInSavingMode
                 << "起始电能:" << savingStartEnergy;
    }
}

DeviceRecord JsonStore::getDeviceByCIp(QString ip)
{
    if (m_ipToDevice.contains(ip)) {
        QString deviceNumber = m_ipToDevice.value(ip);
        return m_devices.value(deviceNumber);
    }
    return DeviceRecord();
}

void JsonStore::updateDisconnectedDevice(QString ip)
{
    DeviceRecord last = getDeviceByCIp(ip);
    if(!last.deviceNumber.isEmpty())
    {
        last.connectedStatus = false;
        m_devices.insert(last.deviceNumber,last);
    }

}

void JsonStore::updateConnectedDevice(QString ip)
{
    DeviceRecord last = getDeviceByCIp(ip);
    if(!last.deviceNumber.isEmpty())
    {
        last.connectedStatus = true;
        last.lastUpdate = QDateTime::currentDateTime();
        m_devices.insert(last.deviceNumber,last);
    }
}

DeviceRecord JsonStore::getDeviceByNumber(const QString &deviceNumber) const
{
    if (m_devices.contains(deviceNumber)) {
        return m_devices.value(deviceNumber);
    }
    return DeviceRecord();  // 返回空记录
}

DeviceRecord JsonStore::getDeviceByCard(const QString &cardNumber) const
{
    for (const DeviceRecord &rec : m_devices.values()) {
        if (rec.cardNumber == cardNumber) {
            return rec;
        }
    }
    return DeviceRecord();
}

QMap<QString, DeviceRecord> JsonStore::getAllDevices() const
{
    return m_devices;
}

void JsonStore::removeDevice(const QString &deviceNumber)
{
    if (m_devices.remove(deviceNumber) > 0) {
        qDebug() << " 删除设备:" << deviceNumber;
        saveToFile();
    }
}

void JsonStore::clearAll()
{
    m_devices.clear();
    qDebug() << " 清空所有设备数据";
    saveToFile();
}

bool JsonStore::saveToFile()
{
    QJsonArray array;
    for (const DeviceRecord &rec : m_devices.values()) {
        array.append(rec.toJson());
    }

    QJsonDocument doc(array);
    QByteArray data = doc.toJson(QJsonDocument::Indented);  // 格式化输出

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << " 保存失败:" << file.errorString();
        emit dataSaved(false);
        return false;
    }

    file.write(data);
    file.close();

    /*qDebug() << "[全数据文件保存]" << m_devices.size()
             << "条记录保存到" << m_filePath;*/
    emit dataSaved(true);
    return true;
}

bool JsonStore::loadFromFile()
{
    QFile file(m_filePath);
    if (!file.exists()) {
        qDebug() << " 数据文件不存在，将创建新文件";
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << " 加载失败:" << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qDebug() << " 数据格式错误";
        return false;
    }

    QJsonArray array = doc.array();
    for (const QJsonValue &val : array) {
        if (val.isObject()) {
            DeviceRecord rec = DeviceRecord::fromJson(val.toObject());
            m_devices.insert(rec.deviceNumber, rec);

            qDebug() << rec.clientIp;
            if(!rec.clientIp.isEmpty())
            {
                m_ipToDevice.insert(rec.clientIp,rec.deviceNumber);
            }
        }
    }

    qInfo() << "[加载文件]"
            << "  设备数量:" << m_devices.size()
            << "  ip绑定数量:" << m_ipToDevice.size();
    return true;
}

int JsonStore::deviceCount() const
{
    return m_devices.size();
}

void JsonStore::updateDeviceLastUpdate(const QString &deviceNumber)
{
    if (m_devices.contains(deviceNumber)) {
        m_devices[deviceNumber].lastUpdate = QDateTime::currentDateTime();
        saveToFile();
    }
}
