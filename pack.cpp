#include "pack.h"

Pack::Pack()
{

}

bool Pack::appendData(const QByteArray &data)
{
    m_buffer.append(data);
    return true;
}


bool Pack::parsePacket(PowerData& parseData)
{
    while(!m_buffer.isEmpty() || m_buffer.size() >= 20)
    {
        int headPos = findFrameHead();
        if (headPos < 0) {
            m_buffer.clear();
            return false;
        }

        if (headPos > 0) {
            m_buffer.remove(0, headPos);
        }

        if (m_buffer.size() < 6) {
            return false;
        }
        // 先读设备ID位判断包类型
        uint8_t devId = static_cast<uint8_t>(m_buffer.at(3));
        qDebug() << "DevId: " << devId  << "   and JNZM_D: " << JNZM_D
                 << "JNZM_D:" << QString("0x%1").arg(JNZM_D, 2, 16, QChar('0')).toUpper();;
        switch(devId)
        {
            case CNG_D:
                parseCNGDevice();
                break;

            case JNZM_D:
                parseJNZMDevice();
                break;

            case JNKT_D:
                parseJNKTDevice();
                break;
        }
        m_buffer.clear();

    }
    parseData = out_pack;
    return true;
}

quint8 Pack::calcXorChecksum(const QByteArray &data, int start, int end)
{
    quint8 checksum = 0;
    for (int i = start; i <= end  && i < data.size(); ++i) {
        checksum ^= static_cast<quint8>(data.at(i));
    }
    return checksum;
}

quint16 Pack::parseUInt16BE(const QByteArray &ba, int offset)
{
    return (static_cast<quint16>(static_cast<quint8>(ba.at(offset))) << 8) |
            static_cast<quint16>(static_cast<quint8>(ba.at(offset + 1)));
}

uint32_t Pack::parseUInt32_BE(const QByteArray &ba, int offset)
{
    return (static_cast<uint32_t>(static_cast<uint8_t>(ba.at(offset)))     << 24) |
           (static_cast<uint32_t>(static_cast<uint8_t>(ba.at(offset + 1))) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(ba.at(offset + 2))) << 8)  |
           (static_cast<uint32_t>(static_cast<uint8_t>(ba.at(offset + 3))));
}

void Pack::printsData(const QString &description)
{
    qDebug() << QString("=== %1 ===").arg(description);
    qDebug() << "=== 数据帧解析 ===";
        qDebug() << "有效:" << out_pack.isValid << "错误:" << out_pack.errorMsg;
        qDebug() << "时间:" << out_pack.timestamp.toString("hh:mm:ss.zzz");
        qDebug() << "服务器:" << QString("0x%1").arg(out_pack.server, 2, 16, QChar('0'));
        qDebug() << "设备ID:" << QString("0x%1").arg(out_pack.deviceId, 2, 16, QChar('0'));
        qDebug() << "功能码:" << QString("0x%1").arg(out_pack.funcCode, 2, 16, QChar('0'));
        qDebug() << "标志:" << QString("0x%1").arg(out_pack.flag, 2, 16, QChar('0'))
                 << (out_pack.flag == 0x01 ? "(无人)" : out_pack.flag == 0x00 ? "(有人)" : "(未知)");
        qDebug() << "拓展:" << QString("0x%1").arg(out_pack.extend, 4, 16, QChar('0'));
        qDebug() << "设备号:" << out_pack.deviceNumber << "卡号:" << out_pack.cardNumber;
        qDebug() << "地址码:" << out_pack.addressCode.toHex(' ').toUpper();
        qDebug() << "电压:" << out_pack.voltage << "    V  电流:" << out_pack.current << "    A  功率:" << out_pack.power << "W";
        qDebug() << "电能:" << out_pack.energy << "KWh  功率因数:" << out_pack.powerFactor;
        qDebug() << "CO2:" << out_pack.co2 << "    Kg  温度:" << out_pack.temperature << "    ℃  频率:" << out_pack.frequency << "Hz";
        qDebug() << "校验:" << QString("0x%1").arg(out_pack.checksum, 2, 16, QChar('0'));


        if (out_pack.deviceId == CNG_D) {
            // 储能柜系统专用输出
            qDebug() << "机箱温度:" << out_pack.temperature << "℃";
            qDebug() << "--- 逆变输出 ---";
            qDebug() << "电压:" << out_pack.voltage << "V  电流:" << out_pack.current << "A  功率:" << out_pack.power << "W  电能:" << out_pack.energy << "KWh";
            qDebug() << "--- 国电充电 ---";
            qDebug() << "电压:" << out_pack.gridChargeVoltage << "V  电流:" << out_pack.gridChargeCurrent << "A  功率:" << out_pack.gridChargePower << "W  电能:" << out_pack.gridChargeEnergy << "KWh";
            qDebug() << "--- 电池数据 ---";
            qDebug() << "电量:" << out_pack.batteryLevel << "  电压:" << out_pack.batteryVoltage << "V";
            qDebug() << "充电电流:" << out_pack.batteryChargeCurrent << "A  放电电流:" << out_pack.batteryDischargeCurrent << "A  温度:" << out_pack.batteryTemperature << "℃";
        } else {
            qDebug() << "地址码:" << out_pack.addressCode.toHex(' ').toUpper();
            qDebug() << "电压:" << out_pack.voltage << "    V  电流:" << out_pack.current << "    A  功率:" << out_pack.power << "W";
            qDebug() << "电能:" << out_pack.energy << "KWh  功率因数:" << out_pack.powerFactor;
            qDebug() << "CO2:" << out_pack.co2 << "    Kg  温度:" << out_pack.temperature << "    ℃  频率:" << out_pack.frequency << "Hz";
        }
        qDebug() << "校验:" << QString("0x%1").arg(out_pack.checksum, 2, 16, QChar('0'));
        qDebug() << "==================";
}


bool Pack::parseSomeonePacket()
{
    out_pack.server = m_buffer.at(2);
    out_pack.deviceId = m_buffer.at(3);
    out_pack.funcCode = m_buffer.at(4);
    out_pack.flag = m_buffer.at(5);
    out_pack.extend = parseUInt16BE(m_buffer,6);
    out_pack.deviceNumber = m_buffer.mid(8, 6).toHex().toUpper();
    out_pack.cardNumber = QString::fromLatin1(m_buffer.mid(14, 10));
    out_pack.isValid = true;
    out_pack.errorMsg = nullptr;

    //以下信息有人包没有数据。
    out_pack.addressCode = 0x00;
    out_pack.voltage = 0;
    out_pack.current = 0;
    out_pack.power = 0;
    out_pack.energy = 0;
    out_pack.powerFactor = 0;
    out_pack.co2 = 0;
    out_pack.temperature = 0;
    out_pack.frequency = 0;

    // 储能柜字段清零
    out_pack.powerSupplyStatus = 0;
    out_pack.gridChargeVoltage = 0;
    out_pack.gridChargeCurrent = 0;
    out_pack.gridChargePower = 0;
    out_pack.gridChargeEnergy = 0;
    out_pack.batteryLevel = 0;
    out_pack.batteryVoltage = 0;
    out_pack.batteryChargeCurrent = 0;
    out_pack.batteryDischargeCurrent = 0;
    out_pack.batteryTemperature = 0;

    out_pack.checksum = calcXorChecksum(m_buffer, 3, m_buffer.size() - 2);
    out_pack.timestamp = QDateTime::currentDateTime();
    printsData("节能照明--有人数据解析");
    return true;
}

bool Pack::parseNoonePacket()
{
    out_pack.server = m_buffer.at(2);
    out_pack.deviceId = m_buffer.at(3);
    out_pack.funcCode = m_buffer.at(4);
    out_pack.flag = m_buffer.at(5);
    out_pack.extend = parseUInt16BE(m_buffer,6);
    out_pack.deviceNumber = m_buffer.mid(8, 6).toHex().toUpper();
    out_pack.cardNumber = QString::fromLatin1(m_buffer.mid(14, 10));
    out_pack.timestamp = QDateTime::currentDateTime();
    out_pack.isValid = true;
    out_pack.errorMsg = nullptr;

    //以下信息有人包没有数据。
    out_pack.addressCode = m_buffer.mid(24, 3).toHex(' ').toUpper();
    out_pack.voltage =  getVoltage(parseUInt32_BE(m_buffer, 27));
    out_pack.current = getCurrent(parseUInt32_BE(m_buffer, 31));
    out_pack.power = getPower(parseUInt32_BE(m_buffer,35));
    out_pack.energy = getEnergy(parseUInt32_BE(m_buffer,39));
    out_pack.powerFactor = getPowerFactor(parseUInt32_BE(m_buffer,43));
    out_pack.co2 = getCO2(parseUInt32_BE(m_buffer,47));
    out_pack.temperature = getTemperature(parseUInt32_BE(m_buffer,51));
    out_pack.frequency = getFrequency(parseUInt32_BE(m_buffer,55));

    out_pack.powerSupplyStatus = 0;
    out_pack.gridChargeVoltage = 0;
    out_pack.gridChargeCurrent = 0;
    out_pack.gridChargePower = 0;
    out_pack.gridChargeEnergy = 0;
    out_pack.batteryLevel = 0;
    out_pack.batteryVoltage = 0;
    out_pack.batteryChargeCurrent = 0;
    out_pack.batteryDischargeCurrent = 0;
    out_pack.batteryTemperature = 0;

    out_pack.checksum = calcXorChecksum(m_buffer, 3, m_buffer.size() - 2);

    printsData("节能照明--无人数据解析");
    return true;
}

bool Pack::parseCNGDevice()
{
    qDebug() << "进入储能柜系统解析";

        const int CNG_PACKET_LENGTH = 87;

        if (m_buffer.size() < CNG_PACKET_LENGTH) {
            out_pack.isValid = false;
            out_pack.errorMsg = QString("储能柜数据包长度不足，期望%1字节，实际%2字节")
                                .arg(CNG_PACKET_LENGTH).arg(m_buffer.size());
            qDebug() << out_pack.errorMsg;
            return false;
        }

        int offset = 2;  // 跳过帧头 AA 55

        out_pack.server = static_cast<quint8>(m_buffer.at(offset++));
        out_pack.deviceId = static_cast<quint8>(m_buffer.at(offset++));
        out_pack.funcCode = static_cast<quint8>(m_buffer.at(offset++));

        // 标志字段（固定0x10）
        out_pack.flag = static_cast<quint8>(m_buffer.at(offset++));

        // 拓展
        out_pack.extend = parseUInt16BE(m_buffer, offset);
        offset += 2;

        // 设备号
        out_pack.deviceNumber = m_buffer.mid(offset, 6).toHex().toUpper();
        offset += 6;

        // 卡号
        out_pack.cardNumber = QString::fromLatin1(m_buffer.mid(offset, 10));
        offset += 10;

        // 系统供电状态（注意：不是flag，是独立的字节！）
        out_pack.powerSupplyStatus = static_cast<quint8>(m_buffer.at(offset++));

        // 机箱温度
        out_pack.temperature = static_cast<double>(m_buffer.at(offset++));

        // 逆变输出数据
        out_pack.voltage = getVoltage(parseUInt32_BE(m_buffer, offset));      // 逆变输出电压
        offset += 4;
        out_pack.current = getCurrent(parseUInt32_BE(m_buffer, offset));      // 逆变输出电流
        offset += 4;
        out_pack.power = getPower(parseUInt32_BE(m_buffer, offset));          // 逆变输出功率
        offset += 4;
        out_pack.energy = getEnergy(parseUInt32_BE(m_buffer, offset));        // 逆变输出电能
        offset += 4;

        // 国电充电数据
        out_pack.gridChargeVoltage = getVoltage(parseUInt32_BE(m_buffer, offset));
        offset += 4;
        out_pack.gridChargeCurrent = getCurrent(parseUInt32_BE(m_buffer, offset));
        offset += 4;
        out_pack.gridChargePower = getPower(parseUInt32_BE(m_buffer, offset));
        offset += 4;
        out_pack.gridChargeEnergy = getEnergy(parseUInt32_BE(m_buffer, offset));
        offset += 4;

        // 电池数据
        out_pack.batteryLevel = parseUInt16BE(m_buffer, offset);
        offset += 2;
        out_pack.batteryVoltage = getCNG_BatteryVoltage(parseUInt16BE(m_buffer, offset));
        offset += 2;
        out_pack.batteryChargeCurrent = getCNG_BatteryCurrent(parseUInt16BE(m_buffer, offset));
        offset += 2;
        out_pack.batteryDischargeCurrent = getCNG_BatteryCurrent(parseUInt16BE(m_buffer, offset));
        offset += 2;
        out_pack.batteryTemperature = getCNG_BatteryTemp(parseUInt16BE(m_buffer, offset));
        offset += 2;

        // 跳过预留字段 (8个 x 2字节 = 16字节)
        offset += 16;

        // 校验值在索引84
        out_pack.checksum = static_cast<quint8>(m_buffer.at(offset));

        // 计算校验：从服务器[2]到预留最后一个字节[83]，即索引 2~83
        quint8 calcChecksum = calcXorChecksum(m_buffer, 2, offset);

        // 帧尾检查
        if (offset + 2 < m_buffer.size()) {
            quint8 tail1 = static_cast<quint8>(m_buffer.at(offset + 1));
            quint8 tail2 = static_cast<quint8>(m_buffer.at(offset + 2));
            if (tail1 != 0x55 || tail2 != 0xAA) {
                qDebug() << "[CNG] 帧尾错误:" << QString("0x%1%2").arg(tail1, 2, 16, QChar('0')).arg(tail2, 2, 16, QChar('0'));
            }
        }

        out_pack.timestamp = QDateTime::currentDateTime();

        if (out_pack.checksum != calcChecksum) {
            out_pack.isValid = false;
            out_pack.errorMsg = QString("校验失败: 计算值0x%1, 实际值0x%2")
                                .arg(calcChecksum, 2, 16, QChar('0'))
                                .arg(out_pack.checksum, 2, 16, QChar('0'));
            qDebug() << "[CNG]" << out_pack.errorMsg;
            // 注意：即使校验失败，也继续解析数据用于调试
            // 实际使用时可以 return false;
        }

        // 清零照明设备专用字段
        out_pack.addressCode.clear();
        out_pack.powerFactor = 0;
        out_pack.co2 = 0;
        out_pack.frequency = 0;

        out_pack.isValid = true;
        out_pack.errorMsg = nullptr;

        printsData("储能柜系统数据解析");
        return true;
}

bool Pack::parseJNZMDevice()
{
    qDebug()<< "进入节能照明解析";
    if(m_buffer.at(5) != 0)
    {
        parseNoonePacket();
    }
    else
    {
        parseSomeonePacket();
    }
    return true;

}

bool Pack::parseJNKTDevice()
{
    qDebug()<< "进入节能空调解析";
}

int Pack::findFrameHead()
{
    for (int i = 0; i < m_buffer.size() - 1; ++i) {
        if (static_cast<uint8_t>(m_buffer.at(i)) == FRAME_HEAD[0] &&
                static_cast<uint8_t>(m_buffer.at(i + 1)) == FRAME_HEAD[1]) {
            return i;
        }
    }
    return -1;
}

