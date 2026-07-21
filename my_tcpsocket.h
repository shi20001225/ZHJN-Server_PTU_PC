#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>
#include <QCoreApplication>
#include <QVector>
#include <QMessageBox>
#include <QTimer>

#include "jsonstore.h"
#include "pack.h"
#include "calculation.h"
#include "monthlydatastore.h"
#include "logger.h"

class My_TcpSocket : public QTcpServer
{
    Q_OBJECT
public:
    explicit My_TcpSocket(QObject *parent = nullptr);

    void refreshInitialDisplay();

    bool parsingData_ZM(const DeviceRecord &lastData, const PowerData &currentData);

    bool parsingData_CNG(DeviceRecord &lastData, const PowerData &currentData);

    void processPacket(QTcpSocket* client, const QByteArray &packet);

signals:
    void sig_ReceivedData_KT(QByteArray KT);

    void sig_RefreshTheInterFace_JNZM(const MonthlyData& monthlyData, double yearData);

    void sig_RefreshTheInterFace_CNG(const MonthlyData& monthlyData, double yearData);

    // 按 deviceId 聚合数据刷新信号
    void sig_RefreshAggregatedByDeviceId(int deviceId, const MonthlyData& aggregatedData, double yearlyReducedCO2);
    // 发送今年12个月每月减碳数据
    void sig_MonthlyCO2DataUpdated(int deviceId, const QVector<double>& monthlyCO2List);


public slots:
    void on_NewConnection();
    void on_ReadyRead();
    void on_Disconnect();
    void on_TimerTimeout();

private slots:


private:
    QTcpServer *server;                             // 服务器
    QList<QTcpSocket*> clients;                     // 存放客户端
    QMap<QTcpSocket*, QByteArray> clientBuffers;    // 存放各客户端的缓冲数据
    QTimer *refreshTheInterface_10s;                // 10S更新界面定时器

    JsonStore *deviceDataStore;                     // 存放完整数据
    MonthlyDataStore *monthlyDataStore;             // 存放月节约数据
    Calculation calculationData;                    // 计算数据
};

#endif // TCPSOCKET_H
