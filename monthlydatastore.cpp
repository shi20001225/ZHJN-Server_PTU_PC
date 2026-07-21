#include "monthlydatastore.h"

MonthlyDataStore::MonthlyDataStore(const QString &dataDir, QObject *parent)
    : QObject(parent)
    , m_dataDir(dataDir)
{
    QDir dir;
    if (!dir.exists(m_dataDir)) {
        dir.mkpath(m_dataDir);
    }

}

MonthlyDataStore::~MonthlyDataStore()
{
    // 保存所有缓存数据
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const QString &deviceNumber = it.key();
        for (auto jt = it.value().begin(); jt != it.value().end(); ++jt) {
            save(deviceNumber, jt.key());
        }
    }
}

void MonthlyDataStore::addData(const QString &deviceNumber,
                               const QString &yearMonth,
                               int deviceId,
                               double savedEnergy,
                               double greenEnergy,
                               double savedCost,
                               double reducedCO2)
{
    // 1. 加载已有数据（如果不在缓存中）
    if (!m_cache.contains(deviceNumber) || !m_cache[deviceNumber].contains(yearMonth)) {
        loadFromFile(deviceNumber, yearMonth);
    }

    // 2. 获取或创建数据
    MonthlyData &data = m_cache[deviceNumber][yearMonth];
    if (data.deviceNumber.isEmpty()) {
        data.deviceNumber = deviceNumber;
        data.yearMonth = yearMonth;
    }

    data.deviceId = deviceId;


    // 3. 累加数据
    data.savedEnergy += savedEnergy;
    data.greenEnergy += greenEnergy;
    data.savedCost += savedCost;
    data.reducedCO2 += reducedCO2;
    data.updateCount++;
    data.lastUpdate = QDateTime::currentDateTime();

    // 4. 保存到文件
    save(deviceNumber, yearMonth);

    /*qDebug() << "[节约文件保存]" << deviceNumber << yearMonth
             << "设备类型:" << QString("0x%1").arg(deviceId, 2, 16, QChar('0')).toUpper()
             << "节约用电:" << data.savedEnergy << "KWh"
             << "绿电:" << data.greenEnergy << "KWh"
             << "节约成本:" << data.savedCost << "元"
             << "减碳:" << data.reducedCO2 << "Kg";*/
}

void MonthlyDataStore::addData(const QString &deviceNumber,
                               int deviceId,
                               double savedEnergy,
                               double greenEnergy,
                               double savedCost,
                               double reducedCO2)
{
    addData(deviceNumber, getCurrentYearMonth(), deviceId, savedEnergy, greenEnergy, savedCost, reducedCO2);
}

MonthlyData MonthlyDataStore::getAggregatedDataByDeviceId(int deviceId) const
{
    MonthlyData aggregated;
    aggregated.deviceId = deviceId;
    aggregated.yearMonth = getCurrentYearMonth();
    aggregated.deviceNumber = QString("AGGREGATED_0x%1").arg(deviceId, 2, 16, QChar('0')).toUpper();

    // 步骤1：收集所有设备号
    QStringList deviceNumbers;
    QDir rootDir(m_dataDir);
    if (rootDir.exists()) {
        deviceNumbers = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    }

    // 步骤2：加载所有设备的所有月份数据（不在遍历 m_cache 时修改它）
    MonthlyDataStore *nonConst = const_cast<MonthlyDataStore*>(this);
    for (const QString &deviceNumber : deviceNumbers) {
        QDir dir(m_dataDir + "/" + deviceNumber);
        if (dir.exists()) {
            for (const QString &file : dir.entryList(QStringList() << "*.json", QDir::Files)) {
                QString ym = file;
                ym = ym.replace(".json", "");
                nonConst->loadFromFile(deviceNumber, ym);
            }
        }
    }

    // 步骤3：累加当前月的数据
    QString currentMonth = getCurrentYearMonth();
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const QMap<QString, MonthlyData> &deviceData = it.value();
        auto jt = deviceData.find(currentMonth);
        if (jt != deviceData.end() && jt.value().deviceId == deviceId) {
            aggregated.savedEnergy += jt.value().savedEnergy;
            aggregated.greenEnergy += jt.value().greenEnergy;
            aggregated.savedCost += jt.value().savedCost;
            aggregated.reducedCO2 += jt.value().reducedCO2;
            aggregated.updateCount += jt.value().updateCount;
        }
    }

    return aggregated;
}

MonthlyData MonthlyDataStore::getYearlyAggregatedDataByDeviceId(int deviceId, int year) const
{
    MonthlyData aggregated;
    aggregated.deviceId = deviceId;
    aggregated.yearMonth = QString("%1-ALL").arg(year);
    aggregated.deviceNumber = QString("AGGREGATED_0x%1").arg(deviceId, 2, 16, QChar('0')).toUpper();

    // 步骤1：收集所有设备号
    QStringList deviceNumbers;
    QDir rootDir(m_dataDir);
    if (rootDir.exists()) {
        deviceNumbers = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    }

    // 步骤2：加载所有数据
    MonthlyDataStore *nonConst = const_cast<MonthlyDataStore*>(this);
    for (const QString &deviceNumber : deviceNumbers) {
        QDir dir(m_dataDir + "/" + deviceNumber);
        if (dir.exists()) {
            for (const QString &file : dir.entryList(QStringList() << "*.json", QDir::Files)) {
                QString ym = file;
                ym = ym.replace(".json", "");
                nonConst->loadFromFile(deviceNumber, ym);
            }
        }
    }

    // 步骤3：累加当年所有月份
    QString prefix = QString("%1-").arg(year);
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const QMap<QString, MonthlyData> &deviceData = it.value();
        for (auto jt = deviceData.begin(); jt != deviceData.end(); ++jt) {
            if (jt.key().startsWith(prefix) && jt.value().deviceId == deviceId) {
                aggregated.savedEnergy += jt.value().savedEnergy;
                aggregated.greenEnergy += jt.value().greenEnergy;
                aggregated.savedCost += jt.value().savedCost;
                aggregated.reducedCO2 += jt.value().reducedCO2;
                aggregated.updateCount += jt.value().updateCount;
            }
        }
    }

    return aggregated;
}

MonthlyData MonthlyDataStore::getYearlyAggregatedDataByDeviceId(int deviceId) const
{
    int currentYear = QDateTime::currentDateTime().date().year();
    return getYearlyAggregatedDataByDeviceId(deviceId, currentYear);
}


MonthlyData MonthlyDataStore::getData(const QString &deviceNumber, const QString &yearMonth) const
{
    if (m_cache.contains(deviceNumber) && m_cache[deviceNumber].contains(yearMonth)) {
        return m_cache[deviceNumber][yearMonth];
    }

    // 尝试从文件加载
    MonthlyDataStore *nonConst = const_cast<MonthlyDataStore*>(this);
    if (nonConst->loadFromFile(deviceNumber, yearMonth)) {
        return m_cache[deviceNumber][yearMonth];
    }

    return MonthlyData();
}

MonthlyData MonthlyDataStore::getCurrentMonthData(const QString &deviceNumber) const
{
    return getData(deviceNumber, getCurrentYearMonth());
}

QMap<QString, MonthlyData> MonthlyDataStore::getYearData(const QString &deviceNumber, int year) const
{
    QMap<QString, MonthlyData> result;
    QString prefix = QString("%1-").arg(year);

    // 从缓存中查找
    if (m_cache.contains(deviceNumber)) {
        for (auto it = m_cache[deviceNumber].begin(); it != m_cache[deviceNumber].end(); ++it) {
            if (it.key().startsWith(prefix)) {
                result.insert(it.key(), it.value());
            }
        }
    }

    // 从文件补充
    QDir dir(m_dataDir + "/" + deviceNumber);
    if (dir.exists()) {
        QStringList filters;
        filters << prefix + "*.json";
        for(const QString &file : dir.entryList(filters, QDir::Files)) {
            QString ym = file;
            ym = ym.replace(".json", "");
            if (!result.contains(ym)) {
                MonthlyDataStore *nonConst = const_cast<MonthlyDataStore*>(this);
                nonConst->loadFromFile(deviceNumber, ym);
                if (m_cache[deviceNumber].contains(ym)) {
                    result.insert(ym, m_cache[deviceNumber][ym]);
                }
            }
        }
    }

    return result;
}

QMap<QString, MonthlyData> MonthlyDataStore::getYearData(const QString &deviceNumber) const
{
    int currentYear = QDateTime::currentDateTime().date().year();
    return getYearData(deviceNumber,currentYear);
}


QMap<QString, MonthlyData> MonthlyDataStore::getAllData(const QString &deviceNumber) const
{
    // 先加载所有文件
    QDir dir(m_dataDir + "/" + deviceNumber);
    if (dir.exists()) {
        MonthlyDataStore *nonConst = const_cast<MonthlyDataStore*>(this);
        for (const QString &file : dir.entryList(QStringList() << "*.json", QDir::Files)) {
            QString ym = file;
            ym = ym.replace(".json", "");
            nonConst->loadFromFile(deviceNumber, ym);
        }
    }

    if (m_cache.contains(deviceNumber)) {
        return m_cache[deviceNumber];
    }

    // 没有数据，返回带默认初始化值的 Map
    QMap<QString, MonthlyData> result;
    MonthlyData data;
    data.deviceNumber = deviceNumber;
    data.yearMonth = getCurrentYearMonth();
    // 其他字段自动初始化为 0（MonthlyData 构造函数）
    result.insert(data.yearMonth, data);


    return result;
}

double MonthlyDataStore::getYearlySavedEnergy(const QString &deviceNumber, int year) const
{
    double total = 0;
    auto data = getYearData(deviceNumber, year);
    for (const MonthlyData &d : data.values()) {
        total += d.savedEnergy;
    }
    return total;
}

double MonthlyDataStore::getYearlyGreenEnergy(const QString &deviceNumber, int year) const
{
    double total = 0;
    auto data = getYearData(deviceNumber, year);
    for (const MonthlyData &d : data.values()) {
        total += d.greenEnergy;
    }
    return total;
}

double MonthlyDataStore::getYearlySavedCost(const QString &deviceNumber, int year) const
{
    double total = 0;
    auto data = getYearData(deviceNumber, year);
    for (const MonthlyData &d : data.values()) {
        total += d.savedCost;
    }
    return total;
}

double MonthlyDataStore::getYearlyReducedCO2(const QString &deviceNumber, int year) const
{
    double total = 0;
    auto data = getYearData(deviceNumber, year);
    for (const MonthlyData &d : data.values()) {
        total += d.reducedCO2;
    }
    return total;
}

void MonthlyDataStore::removeDevice(const QString &deviceNumber)
{
    m_cache.remove(deviceNumber);
    QDir dir(m_dataDir + "/" + deviceNumber);
    if (dir.exists()) {
        dir.removeRecursively();
    }
}

void MonthlyDataStore::clearAll()
{
    m_cache.clear();
    QDir dir(m_dataDir);
    if (dir.exists()) {
        dir.removeRecursively();
        dir.mkpath(m_dataDir);
    }
}

bool MonthlyDataStore::save(const QString &deviceNumber, const QString &yearMonth)
{
    if (!m_cache.contains(deviceNumber) || !m_cache[deviceNumber].contains(yearMonth)) {
        return false;
    }

    QString filePath = getFilePath(deviceNumber, yearMonth);
    QDir dir;
    dir.mkpath(QFileInfo(filePath).path());

    QJsonObject obj = m_cache[deviceNumber][yearMonth].toJson();
    QJsonDocument doc(obj);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[Store] 保存失败:" << file.errorString();
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

QVector<double> MonthlyDataStore::getMonthlyReducedCO2ByDeviceId(int deviceId, int year) const
{
    // 如果 year=0，使用当前年份
    if (year == 0) {
        year = QDateTime::currentDateTime().date().year();
    }

    // 初始化12个月为0
    QVector<double> monthlyCO2(12, 0.0);

    // 步骤1：收集所有设备号
    QStringList deviceNumbers;
    QDir rootDir(m_dataDir);
    if (rootDir.exists()) {
        deviceNumbers = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    }

    // 步骤2：加载所有设备的所有月份数据
    MonthlyDataStore *nonConst = const_cast<MonthlyDataStore*>(this);
    for (const QString &deviceNumber : deviceNumbers) {
        QDir dir(m_dataDir + "/" + deviceNumber);
        if (dir.exists()) {
            for (const QString &file : dir.entryList(QStringList() << "*.json", QDir::Files)) {
                QString ym = file;
                ym = ym.replace(".json", "");
                nonConst->loadFromFile(deviceNumber, ym);
            }
        }
    }

    // 步骤3：遍历所有设备，累加指定 deviceId 的每月减碳
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const QMap<QString, MonthlyData> &deviceData = it.value();
        for (auto jt = deviceData.begin(); jt != deviceData.end(); ++jt) {
            const QString &yearMonth = jt.key();  // 如 "2026-05"
            const MonthlyData &data = jt.value();

            // 检查年份和 deviceId
            if (data.deviceId == deviceId && yearMonth.startsWith(QString::number(year))) {
                // 提取月份（"2026-05" → 5）
                int month = yearMonth.split("-")[1].toInt();
                if (month >= 1 && month <= 12) {
                    monthlyCO2[month - 1] += data.reducedCO2;
                }
            }
        }
    }

    return monthlyCO2;
}

QString MonthlyDataStore::getFilePath(const QString &deviceNumber, const QString &yearMonth) const
{
    return QString("%1/%2/%3.json").arg(m_dataDir).arg(deviceNumber).arg(yearMonth);
}

QString MonthlyDataStore::getCurrentYearMonth() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM");
}

bool MonthlyDataStore::loadFromFile(const QString &deviceNumber, const QString &yearMonth)
{
    QString filePath = getFilePath(deviceNumber, yearMonth);

    QFile file(filePath);
    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return false;
    }

    MonthlyData md = MonthlyData::fromJson(doc.object());
    m_cache[deviceNumber][yearMonth] = md;

    return true;
}
