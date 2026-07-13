#include "my_tcpsocket.h"

My_TcpSocket::My_TcpSocket(QObject *parent)
    : QTcpServer(parent)
{
    QString appDir = QCoreApplication::applicationDirPath();

    deviceDataStore = new JsonStore(appDir + "/realTimeDeviceData.json",this);
    monthlyDataStore = new MonthlyDataStore(appDir + "/monthly_Save_data",this);

    server = new QTcpServer(this);
    server->listen(QHostAddress::AnyIPv4,5540);

    QObject::connect(server,&QTcpServer::newConnection,this,&My_TcpSocket::onNewConnection);

}

// 初始化内容显示
void My_TcpSocket::refreshInitialDisplay()
{
    // ========== 节能照明系统初始显示 ==========
    // 月度聚合
    MonthlyData monthlyAgg = monthlyDataStore->getAggregatedDataByDeviceId(JNZM_D);
    // 年度聚合
    MonthlyData yearlyAgg = monthlyDataStore->getYearlyAggregatedDataByDeviceId(JNZM_D);
    emit sig_RefreshAggregatedByDeviceId(JNZM_D,monthlyAgg,yearlyAgg.reducedCO2);

    // 每个月的减少碳排放量
    QVector<double> monthlyCO2 = monthlyDataStore->getMonthlyReducedCO2ByDeviceId(JNZM_D);
    emit sig_MonthlyCO2DataUpdated(JNZM_D, monthlyCO2);

    // ========== 储能柜系统初始显示 ==========
    MonthlyData fghbMonthlyAgg = monthlyDataStore->getAggregatedDataByDeviceId(CNG_D);
    MonthlyData fghbYearlyAgg = monthlyDataStore->getYearlyAggregatedDataByDeviceId(CNG_D);
    emit sig_RefreshAggregatedByDeviceId(CNG_D, fghbMonthlyAgg, fghbYearlyAgg.reducedCO2);

    QVector<double> fghbMonthlyCO2 = monthlyDataStore->getMonthlyReducedCO2ByDeviceId(CNG_D);
    emit sig_MonthlyCO2DataUpdated(CNG_D, fghbMonthlyCO2);
}

//  解析照明数据
bool My_TcpSocket::parsingData_ZM(const DeviceRecord &lastData, const PowerData &currentData)
{
    // 无人状态不需要计算，只需要记录
    if(currentData.flag == 01)
    {
        return false;
    }

    // 计算本次节约信息
    MonthlyData saveData_JNZM = calculationData.calculate_JNZM(lastData, currentData);
    // 加调试
    qDebug() << "计算结果:"
             << "deviceNumber:" << saveData_JNZM.deviceNumber
             << "deviceId:" << QString("0x%1").arg(currentData.deviceId, 2, 16, QChar('0')).toUpper()
             << "savedEnergy:" << saveData_JNZM.savedEnergy
             << "reducedCO2:" << saveData_JNZM.reducedCO2;

    if(!saveData_JNZM.deviceNumber.isEmpty())
    {

        //叠加本月的信息 和 年度减少碳排放量信息
        monthlyDataStore->addData(saveData_JNZM.deviceNumber,
                                  saveData_JNZM.yearMonth,
                                  JNZM_D,
                                  saveData_JNZM.savedEnergy,
                                  saveData_JNZM.greenEnergy,
                                  saveData_JNZM.savedCost,
                                  saveData_JNZM.reducedCO2);

    }
    else
    {
        qDebug() << "计算结果为空，未保存";
    }
    return true;
}

// 解析储能柜数据
bool My_TcpSocket::parsingData_CNG(DeviceRecord &lastData, const PowerData &currentData)
{
    // 储能柜系统：calculate_CNG 会处理状态切换逻辑，并返回本次是否需要保存数据
    MonthlyData saveData_CNG = calculationData.calculate_CNG(lastData, currentData);

    qDebug() << "储能柜计算结果:"
             << "deviceNumber:" << saveData_CNG.deviceNumber
             << "deviceId:" << QString("0x%1").arg(currentData.deviceId, 2, 16, QChar('0')).toUpper()
             << "savedEnergy:" << saveData_CNG.savedEnergy
             << "greenEnergy:" << saveData_CNG.greenEnergy
             << "reducedCO2:" << saveData_CNG.reducedCO2;

    if (!saveData_CNG.deviceNumber.isEmpty()) {
        // 叠加本月的信息
        monthlyDataStore->addData(saveData_CNG.deviceNumber,
                                  saveData_CNG.yearMonth,
                                  CNG_D,
                                  saveData_CNG.savedEnergy,
                                  saveData_CNG.greenEnergy,
                                  saveData_CNG.savedCost,
                                  saveData_CNG.reducedCO2);
        return true;
    }
    else {
        qDebug() << "储能柜计算结果为空，未保存";
    }
    return false;
}

void My_TcpSocket::onNewConnection()
{
    QTcpSocket* client = server->nextPendingConnection();
    clients.append(client);

    QObject::connect(client,&QTcpSocket::readyRead,this,&My_TcpSocket::onReadyRead);
    QObject::connect(client,&QTcpSocket::disconnected,this,&My_TcpSocket::onDisconnect);
}

void My_TcpSocket::onReadyRead()
{

    QByteArray Head = QByteArray::fromHex("AA55");
    QTcpSocket* client = dynamic_cast<QTcpSocket*>(sender());

    QByteArray buf;
    //读取客户端消息，
    buf = client->readAll();

    if(buf.isEmpty())
    {
        qDebug() << "[onReadyRead]-接受信息失败";
        return;
    }
    qDebug() << QString("[%1]--%2--消息：{%3}").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                .arg(client->peerAddress().toString())
                .arg(QString::fromLocal8Bit(buf.toHex().toUpper()));


    QByteArray DataHead = buf.left(2);
    if(DataHead != Head)
    {
        qDebug() << "[onReadyRead]-头文件检测失败！";
        DataHead.clear();
        return;
    }

    Pack clientPack;
    clientPack.appendData(buf);

    //创建JSon存储格式
    PowerData currentData;
    if(clientPack.parsePacket(currentData))
    {
        DeviceRecord lastData= deviceDataStore->getDeviceByNumber(currentData.deviceNumber);

        // 找到设备消息才去计算和保存计算结果
        bool shouldCalculate = false;
        if (currentData.deviceId == CNG_D) {
            shouldCalculate = true;
        } else if(!lastData.deviceNumber.isEmpty())
        {
            shouldCalculate = true;
        }


        if (shouldCalculate)
        {
            switch(currentData.deviceId)
            {
            case CNG_D:{

                // CNG_D 使用储能柜协议

                bool hasSavedData = parsingData_CNG(lastData, currentData);

                // 先保存当前数据（更新电压、电流等实时数据）
                // updateDevice 会保留已有的 isInSavingMode 状态
                deviceDataStore->updateDevice(currentData);

                // 关键：将 calculate_CNG 修改后的状态同步回 deviceDataStore
                // lastData 已被 calculate_CNG 修改（进入或退出节约模式）
                deviceDataStore->updateSavingState(
                            currentData.deviceNumber,
                            lastData.isInSavingMode,
                            lastData.savingStartEnergy,
                            lastData.savingStartTime
                            );

                if (hasSavedData) {
                    emit sig_RefreshAggregatedByDeviceId(CNG_D,
                                                         monthlyDataStore->getAggregatedDataByDeviceId(CNG_D),
                                                         monthlyDataStore->getYearlyAggregatedDataByDeviceId(CNG_D).reducedCO2);

                    QVector<double> monthlyCO2 = monthlyDataStore->getMonthlyReducedCO2ByDeviceId(CNG_D);
                    emit sig_MonthlyCO2DataUpdated(CNG_D, monthlyCO2);
                }
                break;
            }

            case JNZM_D:{
                if(parsingData_ZM(lastData, currentData))
                {
                    emit sig_RefreshAggregatedByDeviceId(JNZM_D,
                                                         monthlyDataStore->getAggregatedDataByDeviceId(JNZM_D),
                                                         monthlyDataStore->getYearlyAggregatedDataByDeviceId(JNZM_D).reducedCO2);
                    // 发射12个月减碳数据
                    QVector<double> monthlyCO2 = monthlyDataStore->getMonthlyReducedCO2ByDeviceId(JNZM_D);
                    emit sig_MonthlyCO2DataUpdated(JNZM_D, monthlyCO2);
                }
                break;
            }


            case JNKT_D:{
                break;
            }


            }

        }
    }
    // 对于非CNG设备，在这里更新
    if (currentData.deviceId != CNG_D) {
        deviceDataStore->updateDevice(currentData);
    }

}
void My_TcpSocket::onDisconnect()
{
    QTcpSocket* client = dynamic_cast<QTcpSocket*>(sender());
    clients.removeAll(client);
    client->deleteLater();
}


