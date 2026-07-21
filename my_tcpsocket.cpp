#include "my_tcpsocket.h"

My_TcpSocket::My_TcpSocket(QObject *parent)
    : QTcpServer(parent)
{
    QString appDir = QCoreApplication::applicationDirPath();

    deviceDataStore = new JsonStore(appDir + "/realTimeDeviceData.json",this);
    monthlyDataStore = new MonthlyDataStore(appDir + "/monthly_Save_data",this);

    server = new QTcpServer(this);
    server->listen(QHostAddress::AnyIPv4,5540);
    QString peerInfo = QString("%1:%2").arg(server->serverAddress().toString()).arg(server->serverPort());
    qInfo() << "[服务器启动]" << peerInfo;
    QObject::connect(server,&QTcpServer::newConnection,this,&My_TcpSocket::on_NewConnection);


    refreshTheInterface_10s = new QTimer(this);
    connect(refreshTheInterface_10s, &QTimer::timeout, this, &My_TcpSocket::on_TimerTimeout);
    refreshTheInterface_10s->start(10000);

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

//  计算照明节约数据
bool My_TcpSocket::parsingData_ZM(const DeviceRecord &lastData, const PowerData &currentData)
{

    if(lastData.flag == 0 || (currentData.flag == 0 && lastData.flag == 0) || !currentData.connectedStatus)
    {
        qInfo() << "[计算]"
                << "  上次无人:"  << lastData.flag
                << "  两次相同:" <<  (currentData.flag == 0 && lastData.flag == 0)
                << "  连接状态:" << lastData.connectedStatus;
        return false;
    }


    // 计算本次节约信息
    MonthlyData saveData_JNZM = calculationData.calculate_JNZM(lastData, currentData);
    // 加调试
    qInfo() << "[计算]"
            << "  设备号:" << saveData_JNZM.deviceNumber
            << "  设备类型:" << currentData.deviceId
            << "  节约电量:" << saveData_JNZM.savedEnergy
            << "  节约月碳排放量:" << saveData_JNZM.reducedCO2;

    if(!saveData_JNZM.deviceNumber.isEmpty())
    {
        //叠加本月的信息
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
        qWarning() << "计算结果为空，未保存";
    }
    return true;
}

// 解析储能柜数据
bool My_TcpSocket::parsingData_CNG(DeviceRecord &lastData, const PowerData &currentData)
{
    if(!currentData.connectedStatus)
    {
        return false;
    }
    // 储能柜系统：calculate_CNG 会处理状态切换逻辑，并返回本次是否需要保存数据
    MonthlyData saveData_CNG = calculationData.calculate_CNG(lastData, currentData);

    qInfo() << QString("储能柜计算结果: \n"
                       " deviceNumber: %1 \n"
                       "deviceId: %2 \n"
                       "savedEnergy: %3 \n"
                       "greenEnergy: %4 \n"
                       "reducedCO2: %5")
               .arg(saveData_CNG.deviceNumber)
               .arg(QString("0x%1").arg(currentData.deviceId, 2, 16, QChar('0')).toUpper())
               .arg(saveData_CNG.savedEnergy)
               .arg(saveData_CNG.greenEnergy)
               .arg(saveData_CNG.reducedCO2);



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
        qWarning() << "储能柜计算结果为空，未保存";
    }
    return false;
}

void My_TcpSocket::processPacket(QTcpSocket *client, const QByteArray &packet)
{
    PowerData currentData;
    Pack clientPack;
    clientPack.appendData(packet);
    QString clientIp = client->peerAddress().toString();

    if(clientPack.parsePacket(currentData, clientIp))
    {
        qInfo() << "[数据解析]" << currentData.ip
                << "设备号:" << currentData.deviceNumber
                << "设备类型:" << currentData.deviceId
                << "标志:" << currentData.flag
                << "功率:" << (currentData.power > 0 ? currentData.power : currentData.voltage * currentData.current)
                << "时间:" << currentData.timestamp.toString("yyyy-MM-dd hh-mm-ss");

        DeviceRecord lastData = deviceDataStore->getDeviceByNumber(currentData.deviceNumber);

        // IP绑定...
        if(deviceDataStore->getDeviceByCIp(currentData.ip).deviceNumber.isEmpty()) {
            deviceDataStore->appendIpToDevice(currentData.ip, currentData.deviceNumber);
        }

        // 计算保存
        bool shouldCalculate = false;
        if (currentData.deviceId == CNG_D) {
            shouldCalculate = true;
        } else if (!lastData.deviceNumber.isEmpty()) {
            shouldCalculate = true;
        }

        if (shouldCalculate) {
            switch(currentData.deviceId) {
            case CNG_D: {
                if (parsingData_CNG(lastData, currentData)) {
                    qInfo() << (QString("解析%1的储能柜节能数据").arg(currentData.deviceNumber));

                    // 更新月节约能量
                    deviceDataStore->updateSavingState(
                                currentData.deviceNumber,
                                lastData.isInSavingMode,
                                lastData.savingStartEnergy,
                                lastData.savingStartTime
                                );

                    emit sig_RefreshAggregatedByDeviceId(CNG_D,
                                                         monthlyDataStore->getAggregatedDataByDeviceId(CNG_D),
                                                         monthlyDataStore->getYearlyAggregatedDataByDeviceId(CNG_D).reducedCO2);

                    QVector<double> monthlyCO2 = monthlyDataStore->getMonthlyReducedCO2ByDeviceId(CNG_D);
                    emit sig_MonthlyCO2DataUpdated(CNG_D, monthlyCO2);

                }
                deviceDataStore->updateDevice(currentData);
                break;
            }
            case JNZM_D: {
                if (parsingData_ZM(lastData, currentData)) {
                    emit sig_RefreshAggregatedByDeviceId(JNZM_D,
                                                         monthlyDataStore->getAggregatedDataByDeviceId(JNZM_D),
                                                         monthlyDataStore->getYearlyAggregatedDataByDeviceId(JNZM_D).reducedCO2);
                    // 发射12个月减碳数据
                    QVector<double> monthlyCO2 = monthlyDataStore->getMonthlyReducedCO2ByDeviceId(JNZM_D);
                    emit sig_MonthlyCO2DataUpdated(JNZM_D, monthlyCO2);
                }
                deviceDataStore->updateDevice(currentData);
                break;
            }
            case JNKT_D: {
                break;
            }
            }
        }
    }
}

void My_TcpSocket::on_TimerTimeout()
{
    // 获取所有设备记录
    QMap<QString, DeviceRecord> devices = deviceDataStore->getAllDevices();

    bool isSaveToFile = false;
    for (auto it = devices.begin(); it != devices.end(); ++it) {
        DeviceRecord &rec = it.value();


        // 照明节能设备（JNZM_D）且为无人状态,并且链接状态
        if (rec.deviceId == JNZM_D && rec.flag == 1 && rec.connectedStatus) {
            // 计算从上次更新到现在的间隔（小时）
            QDateTime currentTime = QDateTime::currentDateTime();
            double hours = rec.lastUpdate.secsTo(currentTime) / 3600.0;
            if (hours <= 0) continue;

            // 节约电量
            double savedEnergy;
            double PW;
            if(rec.power <= 0)
            {
                PW = rec.current * rec.voltage;
                savedEnergy = calculationData.calculateEnergySavings(PW,hours);
            }else
            {
                PW = rec.power;
                savedEnergy = calculationData.calculateEnergySavings(rec.power, hours);
            }


            // 如果有节约量，则存储到月度数据库
            if (savedEnergy > 0) {
                isSaveToFile = true;
                monthlyDataStore->addData(rec.deviceNumber,
                                          JNZM_D,
                                          savedEnergy,
                                          0.0,
                                          savedEnergy * ELECTRICITY_PRICE,
                                          savedEnergy * EMISSION_FACTOR);

                deviceDataStore->updateDeviceLastUpdate(rec.deviceNumber);

                qInfo() << "[定时刷新-照明]"
                        << rec.clientIp
                        << "  设备:" <<rec.deviceNumber
                        << "  无人:" << rec.flag
                        << "  节约电量:" << savedEnergy
                        << "  功率:" << PW
                        << "  时间间隔:" << hours * 3600.0;
            }

        }
        else
        {
            qWarning() << "[定时刷新]"
                       << rec.deviceNumber
                       << "  设备一致:" << (rec.deviceId == JNZM_D)
                       << "  无人:" << (rec.flag == 1)
                       << "  连接状态:" << rec.connectedStatus;
        }
    }

    MonthlyData monthlyAgg = monthlyDataStore->getAggregatedDataByDeviceId(JNZM_D);
    MonthlyData yearlyAgg = monthlyDataStore->getYearlyAggregatedDataByDeviceId(JNZM_D);
    emit sig_RefreshAggregatedByDeviceId(JNZM_D, monthlyAgg, yearlyAgg.reducedCO2);

    QVector<double> monthlyCO2 = monthlyDataStore->getMonthlyReducedCO2ByDeviceId(JNZM_D);
    emit sig_MonthlyCO2DataUpdated(JNZM_D, monthlyCO2);

}

void My_TcpSocket::on_NewConnection()
{
    QTcpSocket* client = server->nextPendingConnection();
    clients.append(client);
    clientBuffers[client].clear();

    qInfo() << "[客户端连接]" << client->peerAddress().toString();

    // 检查这个IP地址是否有绑定设备号，如果有就设置为可以刷新状态
    deviceDataStore->updateConnectedDevice(client->peerAddress().toString());

    QObject::connect(client,&QTcpSocket::errorOccurred,this,[this,client](QAbstractSocket::SocketError socketError){
        qDebug() << "[Socket错误]" << socketError << client->errorString();
    });
    QObject::connect(client,&QTcpSocket::readyRead,this,&My_TcpSocket::on_ReadyRead);
    QObject::connect(client,&QTcpSocket::disconnected,this,&My_TcpSocket::on_Disconnect);
}

void My_TcpSocket::on_ReadyRead()
{
    try {

        QTcpSocket* client = dynamic_cast<QTcpSocket*>(sender());
        if (!client) return;

        QByteArray &buffer = clientBuffers[client];
        QByteArray newData = client->readAll();
        buffer.append(newData);

        qInfo() << "[接收缓冲]" << client->peerAddress().toString()
                        << "本次接收:" << newData.length()
                        << "缓冲总长:" << buffer.length();

        const int MIN_PACKET_LEN = 26; // 最小长度

        while(buffer.length() >= MIN_PACKET_LEN)
        {
            int headPos = buffer.indexOf(QByteArray("\xAA\x55", 2));
            if(headPos < 0)
            {
                qWarning() << "[无包头] 丢弃" << buffer.length() << "字节";
                buffer.clear();
                return;
            }

            if(headPos > 0)
            {
                qWarning() << "[跳过垃圾] 前" << headPos << "字节:"
                           << QString::fromLocal8Bit(buffer.left(headPos).toHex().toUpper());
                buffer.remove(0,headPos);
            }

            if(buffer.length() < 6)
            {
                qInfo() << "[半包] 不够判断包类型，等待更多数据";
                return;
            }

            uint8_t deviceId = static_cast<uint8_t>(buffer.at(3));
            int packLen = 0;

            switch(deviceId)
            {
                case CNG_D: // 0x71
                    packLen = 87;
                    break;

                case JNZM_D: //0x81
                    if(buffer.length() < 6) return;
                    if(static_cast<uint8_t>(buffer.at(5)) == 0)
                    {
                        packLen = 27;
                    } else
                    {
                        packLen = 62;
                    }
                    break;

                case JNKT_D: //0xxx
                    break;

                default:
                    qWarning() << "[位置设备ID]" << QString("0x%1").arg(deviceId, 2, 16, QChar('0'));
                    buffer.remove(0, 2);  // 跳过这个假包头，继续找
                    continue;
            }

            if(buffer.length() < packLen)
            {
                qInfo() << "[半包] 需要" << packLen << "字节,当前" << buffer.length() << "字节";
                return;
            }

            QByteArray packet = buffer.left(packLen);
            buffer.remove(0, packLen);

            QByteArray tail = packet.right(2);
            if (tail != QByteArray("\x55\xAA", 2)) {
                qWarning() << "[包尾错误] ...";

                // 在已提取的 packet 中找下一个 AA55
                int nextHead = packet.indexOf(QByteArray("\xAA\x55", 2), 2);
                if (nextHead > 0) {
                    QByteArray remaining = packet.mid(nextHead);
                    buffer.prepend(remaining);
                    qInfo() << "[回退数据]" << remaining.length() << "字节回退到缓冲";
                } else {
                    buffer.remove(0, 2);
                }
                continue;
            }

            qInfo() << "[提取完整包] 设备ID:" << QString("0x%1").arg(deviceId, 2, 16, QChar('0')) << "长度:" << packLen;

            processPacket(client, packet);
        }


    } catch (const std::exception &e) {
        qWarning() << "未处理异常:" << e.what();
        return ;

    } catch (...) {
        qWarning() << "未知异常";
        return ;
    }

}

void My_TcpSocket::on_Disconnect()
{
    QTcpSocket* client = dynamic_cast<QTcpSocket*>(sender());
    qInfo() << "[断开连接]" << client->peerAddress().toString()
            << "  错误码:" << client->error()
            << "  错误信息:" << client->errorString();
    deviceDataStore->updateDisconnectedDevice(client->peerAddress().toString());
    clientBuffers.remove(client);
    clients.removeAll(client);
    client->deleteLater();
}
