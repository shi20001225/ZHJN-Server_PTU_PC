#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>
#include <QCoreApplication>
#include <QVector>

#include "jsonstore.h"
#include "pack.h"
#include "calculation.h"
#include "monthlydatastore.h"

class My_TcpSocket : public QTcpServer
{
    Q_OBJECT
public:
    explicit My_TcpSocket(QObject *parent = nullptr);

    void refreshInitialDisplay();

    bool parsingData_ZM(const DeviceRecord &lastData, const PowerData &currentData);

    bool parsingData_CNG(DeviceRecord &lastData, const PowerData &currentData);

signals:
    void sig_ReceivedData_KT(QByteArray KT);

    void sig_RefreshTheInterFace_JNZM(const MonthlyData& monthlyData, double yearData);

    void sig_RefreshTheInterFace_CNG(const MonthlyData& monthlyData, double yearData);

    // 按 deviceId 聚合数据刷新信号
    void sig_RefreshAggregatedByDeviceId(int deviceId, const MonthlyData& aggregatedData, double yearlyReducedCO2);
    // 发送今年12个月每月减碳数据
    void sig_MonthlyCO2DataUpdated(int deviceId, const QVector<double>& monthlyCO2List);


public slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnect();

private slots:


private:
    QTcpServer *server;
    QMap<QString,QString> map;
    QList<QTcpSocket*> clients;

    JsonStore *deviceDataStore;
    MonthlyDataStore *monthlyDataStore;
    Calculation calculationData;
};

#endif // TCPSOCKET_H
