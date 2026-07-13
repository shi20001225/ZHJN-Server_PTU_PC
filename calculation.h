#ifndef CALCULATION_H
#define CALCULATION_H
/********************************
 *
 * 用于计算本次数据发送过来的用电、产电、节约成本、减少碳排放量
 *
 *********************************/


#include <QObject>
#include <QDateTime>

#include "jsonstore.h"
#include "monthlydatastore.h"


const constexpr double ELECTRICITY_PRICE = 1;
const constexpr double EMISSION_FACTOR= 0.8;

class Calculation : public QObject
{
public:
    Calculation(QObject *parent = nullptr);

    // 计算节能照明数据
    MonthlyData calculate_JNZM(const DeviceRecord& lastData,const PowerData& currentData);

    // 计算储能柜数据
    MonthlyData calculate_CNG(DeviceRecord& lastData, const PowerData& currentData);

    // 计算节约用电
    double calculateEnergySavings(const double& power, const double& interval);

    // 计算节约成本
    double calculateCostSavings(const double& SaveElectricity);

    // 计算产生绿电
    double calculateGreenElectricity(const PowerData& lastData,const PowerData& currentData);

    // 计算减少碳排放量
    double reduceCarbonEmissons(const double& saveElectricity);



};

#endif // CALCULATION_H
