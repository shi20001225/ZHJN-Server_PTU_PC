#ifndef MONTHLYDATASTORE_H
#define MONTHLYDATASTORE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QMap>
#include <QDebug>

/**
 *  月度统计数据结构
 */
struct MonthlyData {
    QString deviceNumber;       // 设备号
    QString yearMonth;          // 年月，如 "2026-07"
    int deviceId;               // 设备ID


    double savedEnergy;         // 节约用电 (KWh)
    double greenEnergy;         // 绿电数据 (KWh)
    double savedCost;           // 节约成本 (元)
    double reducedCO2;          // 减少碳排放量 (Kg)

    int updateCount;            // 更新次数
    QDateTime lastUpdate;       // 最后更新时间

    MonthlyData()
        : deviceId(0), savedEnergy(0), greenEnergy(0), savedCost(0), reducedCO2(0), updateCount(0) {}

    // 转成 JSON
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["deviceNumber"] = deviceNumber;
        obj["yearMonth"] = yearMonth;
        obj["deviceId"] = deviceId;
        obj["savedEnergy"] = savedEnergy;
        obj["greenEnergy"] = greenEnergy;
        obj["savedCost"] = savedCost;
        obj["reducedCO2"] = reducedCO2;
        obj["updateCount"] = updateCount;
        obj["lastUpdate"] = lastUpdate.toString("yyyy-MM-dd hh:mm:ss");
        return obj;
    }

    // 从 JSON 解析
    static MonthlyData fromJson(const QJsonObject &obj) {
        MonthlyData data;
        data.deviceNumber = obj["deviceNumber"].toString();
        data.yearMonth = obj["yearMonth"].toString();
        data.deviceId = obj["deviceId"].toInt();
        data.savedEnergy = obj["savedEnergy"].toDouble();
        data.greenEnergy = obj["greenEnergy"].toDouble();
        data.savedCost = obj["savedCost"].toDouble();
        data.reducedCO2 = obj["reducedCO2"].toDouble();
        data.updateCount = obj["updateCount"].toInt();
        data.lastUpdate = QDateTime::fromString(obj["lastUpdate"].toString(), "yyyy-MM-dd hh:mm:ss");
        return data;
    }
};


class MonthlyDataStore : public QObject
{
    Q_OBJECT
public:
    explicit MonthlyDataStore(const QString &dataDir = "monthly_data", QObject *parent = nullptr);
    ~MonthlyDataStore();


    /**
     * @brief 添加数据（自动累加）
     * @param deviceNumber 设备号
     * @param yearMonth 年月 "2026-07"
     * @param savedEnergy 节约用电增量
     * @param greenEnergy 绿电增量
     * @param savedCost 节约成本增量
     * @param reducedCO2 减少碳排放增量
     */
    void addData(const QString &deviceNumber,
                 const QString &yearMonth,
                 int deviceId,
                 double savedEnergy,
                 double greenEnergy,
                 double savedCost,
                 double reducedCO2);

    /**
     * @brief 添加数据（自动获取当前年月）
     */
    void addData(const QString &deviceNumber,
                 int deviceId,
                 double savedEnergy,
                 double greenEnergy,
                 double savedCost,
                 double reducedCO2);

    // ========== 查询方法 ==========

    // 获取指定设备某月数据
    MonthlyData getData(const QString &deviceNumber, const QString &yearMonth) const;

    // 获取指定设备当前月数据
    MonthlyData getCurrentMonthData(const QString &deviceNumber) const;

    // 获取指定设备某年所有月份数据
    QMap<QString, MonthlyData> getYearData(const QString &deviceNumber, int year) const;

    // 获取制定设备的当年的数据
    QMap<QString, MonthlyData> getYearData(const QString &deviceNumber) const;

    // 获取指定设备所有历史数据
    QMap<QString, MonthlyData> getAllData(const QString &deviceNumber) const;

    /**
     * @brief 获取指定 deviceId 的所有设备累加数据（当前月）
     * @param deviceId 设备类型ID，如 0x81
     * @return 累加后的 MonthlyData
     */
    MonthlyData getAggregatedDataByDeviceId(int deviceId) const;

    /**
     * @brief 获取指定 deviceId 的所有设备年度累加数据
     * @param deviceId 设备类型ID
     * @param year 年份
     * @return 累加后的 MonthlyData
     */
    MonthlyData getYearlyAggregatedDataByDeviceId(int deviceId, int year) const;

    /**
     * @brief 获取指定 deviceId 的所有设备年度累加数据（当年）
     */
    MonthlyData getYearlyAggregatedDataByDeviceId(int deviceId) const;

    // ========== 年度统计 ==========

    // 年累计节约用电
    double getYearlySavedEnergy(const QString &deviceNumber, int year) const;

    // 年累计绿电
    double getYearlyGreenEnergy(const QString &deviceNumber, int year) const;

    // 年累计节约成本
    double getYearlySavedCost(const QString &deviceNumber, int year) const;

    // 年累计减少碳排放
    double getYearlyReducedCO2(const QString &deviceNumber, int year) const;

    // ========== 管理方法 ==========

    // 删除设备所有数据
    void removeDevice(const QString &deviceNumber);

    // 清空所有数据
    void clearAll();

    // 手动保存
    bool save(const QString &deviceNumber, const QString &yearMonth);

    // 获取今年每月减碳数据
    QVector<double> getMonthlyReducedCO2ByDeviceId(int deviceId, int year = 0) const;

signals:
    void dataAdded(const QString &deviceNumber, const QString &yearMonth, int deviceId);

    // 按 deviceId 聚合数据更新信号
    void aggregatedDataUpdated(int deviceId, const MonthlyData &aggregatedData, double yearlyReducedCO2);

    void monthlyCO2DataUpdated(int deviceId, const QVector<double> &monthlyCO2List);

private:
    QString m_dataDir;

    // 缓存：deviceNumber -> yearMonth -> MonthlyData
    QMap<QString, QMap<QString, MonthlyData>> m_cache;

    QString getFilePath(const QString &deviceNumber, const QString &yearMonth) const;
    QString getCurrentYearMonth() const;
    bool loadFromFile(const QString &deviceNumber, const QString &yearMonth);
};

#endif // MONTHLYDATASTORE_H
