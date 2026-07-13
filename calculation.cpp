#include "calculation.h"

Calculation::Calculation(QObject *parent)
{

}

MonthlyData Calculation::calculate_JNZM(const DeviceRecord &lastData, const PowerData &currentData)
{
    MonthlyData thisEnergySavingData;
    if (currentData.deviceId != JNZM_D) {
        return thisEnergySavingData;
    }

    if(lastData.flag != 1 || currentData.flag != 0)
    {
        return thisEnergySavingData;
    }
    thisEnergySavingData.deviceNumber = currentData.deviceNumber;
    thisEnergySavingData.yearMonth = QDateTime::currentDateTime().toString("yyyy-MM");
    thisEnergySavingData.deviceId = currentData.deviceId;

    //计算秒级时间差
    double hours = lastData.lastUpdate.secsTo(currentData.timestamp) / 3600.0;


    // 计算节约电量
    if(lastData.power <= 0)
    {
        double Pw = lastData.current * lastData.voltage;
        thisEnergySavingData.savedEnergy = calculateEnergySavings(Pw,hours);
    }else
    {
        thisEnergySavingData.savedEnergy = calculateEnergySavings(lastData.power, hours);
    }

    // 计算节约的成本
    thisEnergySavingData.savedCost = calculateCostSavings(thisEnergySavingData.savedEnergy);
    // 计算减少的碳排放量
    thisEnergySavingData.reducedCO2 = reduceCarbonEmissons(thisEnergySavingData.savedEnergy);

    //本节无用数据
    thisEnergySavingData.greenEnergy = 0.0;
    thisEnergySavingData.updateCount = 0;
    thisEnergySavingData.lastUpdate = QDateTime::currentDateTime();

    return thisEnergySavingData;
}

MonthlyData Calculation::calculate_CNG(DeviceRecord &lastData, const PowerData &currentData)
{
    MonthlyData thisData;

    // 基本校验：设备ID必须是风光互补系统
    if (currentData.deviceId != CNG_D) {
        return thisData;
    }

    thisData.deviceNumber = currentData.deviceNumber;
    thisData.yearMonth = QDateTime::currentDateTime().toString("yyyy-MM");
    thisData.deviceId = currentData.deviceId;

    quint8 lastStatus = lastData.powerSupplyStatus;
    quint8 currentStatus = currentData.powerSupplyStatus;

    qDebug() << "[CNG计算] 设备:" << currentData.deviceNumber
             << "上次供电状态:" << lastStatus << "(1=国电,2=风光,3=电池)"
             << "本次供电状态:" << currentStatus
             << "上次是否节约中:" << lastData.isInSavingMode
             << "上次起始电能:" << lastData.savingStartEnergy;

    // ========== 场景1：国电 -> 非国电（开始节约模式）==========
    if (lastStatus == 1 && currentStatus != 1) {
        lastData.isInSavingMode = true;
        lastData.savingStartEnergy = currentData.energy;  // 记录开始时的逆变器电能
        lastData.savingStartTime = QDateTime::currentDateTime();

        qDebug() << "[CNG计算] 国电->非国电，开始节约模式，起始电能:" << currentData.energy << "KWh";

        // 本次不产生节约数据，只记录状态
        thisData.deviceNumber.clear();  // 返回空，不保存
        return thisData;
    }

    // ========== 场景2：非国电 -> 非国电（持续节约中）==========
    if (lastStatus != 1 && currentStatus != 1) {
        // 如果之前没有进入节约模式（首次上线直接就是非国电），补录起始状态
        if (!lastData.isInSavingMode) {
            lastData.isInSavingMode = true;
            lastData.savingStartEnergy = currentData.energy;
            lastData.savingStartTime = QDateTime::currentDateTime();
            qDebug() << "[CNG计算] 首次非国电，补录节约模式，起始电能:" << currentData.energy << "KWh";
        } else {
            qDebug() << "[CNG计算] 持续非国电供电中，暂不计算";
        }

        thisData.deviceNumber.clear();  // 返回空，不保存
        return thisData;
    }

    // ========== 场景3：非国电 -> 国电（结束节约模式，计算节约电量）==========
    if (lastStatus != 1 && currentStatus == 1) {
        if (lastData.isInSavingMode) {
            // 计算节约电量 = 结束时的电能 - 开始时的电能
            double savedEnergy = currentData.energy - lastData.savingStartEnergy;

            if (savedEnergy > 0) {
                thisData.savedEnergy = savedEnergy;
                thisData.savedCost = calculateCostSavings(savedEnergy);
                thisData.reducedCO2 = reduceCarbonEmissons(savedEnergy);

                // 绿电：这段时间内使用的是风光/电池供电，全部算绿电
                thisData.greenEnergy = savedEnergy;

                qDebug() << "[CNG计算] 非国电->国电，结束节约模式"
                         << "结束电能:" << currentData.energy << "KWh"
                         << "起始电能:" << lastData.savingStartEnergy << "KWh"
                         << "节约电量:" << savedEnergy << "KWh"
                         << "节约成本:" << thisData.savedCost << "元"
                         << "减碳:" << thisData.reducedCO2 << "Kg";
            } else {
                qDebug() << "[CNG计算] 节约电量非正数，不保存:" << savedEnergy;
                thisData.deviceNumber.clear();
            }

            // 重置节约模式状态
            lastData.isInSavingMode = false;
            lastData.savingStartEnergy = 0.0;

        } else {
            // 之前没有进入节约模式，可能是首次上线就是国电状态
            qDebug() << "[CNG计算] 非国电->国电，但之前未记录节约模式，无法计算";
            thisData.deviceNumber.clear();
        }

        thisData.updateCount = 0;
        thisData.lastUpdate = QDateTime::currentDateTime();
        return thisData;
    }

    // ========== 场景4：国电 -> 国电（无变化）==========
    // lastStatus == 1 && currentStatus == 1
    qDebug() << "[CNG计算] 国电->国电，无变化";
    thisData.deviceNumber.clear();  // 返回空，不保存
    return thisData;
}

double Calculation::calculateEnergySavings(const double &power, const double &interval)
{
    return power * interval / 1000.0;
}

double Calculation::calculateCostSavings(const double& SaveElectricity)
{
    return SaveElectricity * ELECTRICITY_PRICE;
}

double Calculation::calculateGreenElectricity(const PowerData& lastData,const PowerData& currentData)
{

}

double Calculation::reduceCarbonEmissons(const double& saveElectricity)
{
    return saveElectricity * EMISSION_FACTOR;
}
