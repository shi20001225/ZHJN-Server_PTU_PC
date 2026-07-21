#ifndef PACK_H
#define PACK_H

#include <QDebug>
#include <QString>
#include <QByteArray>
#include <QDateTime>

static constexpr uint8_t FRAME_HEAD[] = {0xAA, 0x55};
const constexpr uint8_t FRAME_TAIL[] = {0x55, 0xAA};

enum Device{
    CNG_D = 0x71,
    JNZM_D = 0x81,
    JNKT_D = 0x91,
};

struct PowerData {
        QString ip;             // IP地址
        bool connectedStatus;   // 连接状态
        quint8  server;         // 服务器
        quint8  deviceId;       // 设备ID
        quint8  funcCode;       // 功能码
        quint8  flag;           // 标志 (01=无人)
        quint16 extend;         // 拓展
        QString deviceNumber;   // 设备号 (BCD码, 6字节)
        QString cardNumber;     // 卡号 (ASCII, 10位)
        // ========== 节能照明设备系统无人包专用字段 ==========
        // JNZM_D设备（0x71）有效
        QByteArray addressCode; // 地址码 (3字节, 暂不做分析)
        double  voltage;        // 电压 (V)  — 对CNG_D：逆变输出电压v
        double  current;        // 电流 (A)  — 对CNG_D：逆变输出电流
        double  power;          // 功率 (W)  — 对CNG_D：逆变输出功率
        double  energy;         // 电能 (KWh) — 对CNG_D：逆变输出电能
        double  powerFactor;    // 功率因素
        double  co2;            // CO2排量 (Kg)
        double  temperature;    // 温度 (℃) — 对CNG_D：机箱温度
        double  frequency;      // 频率 (Hz)
        quint8  checksum;       // 校验值
        QDateTime timestamp;    // 接收时间

        bool isValid;           // 解析是否有效
        QString errorMsg;       // 错误信息


        // ========== 储能柜系统专用字段 ==========
        // CNG_D设备（0x71）有效
        quint8 powerSupplyStatus;   // 系统供电状态 (1:国电, 2:风光, 3:电池)

        // 国电充电数据
        double gridChargeVoltage;   // 国电充电电压 (V)
        double gridChargeCurrent;   // 国电充电电流 (A)
        double gridChargePower;     // 国电充电功率 (W)
        double gridChargeEnergy;    // 国电充电电能 (KWh)

        // 电池数据
        quint16 batteryLevel;       // 电池电量
        double batteryVoltage;      // 电池电压 (V)
        double batteryChargeCurrent;    // 电池充电电流 (A)
        double batteryDischargeCurrent; // 电池放电电流 (A)
        double batteryTemperature;  // 电池温度 (℃)

        // 预留字段（暂不解析，但跳过）
        // reserved[8]
};

class Pack
{


public:
    Pack();

    bool appendData(const QByteArray& data);
    bool parsePacket(PowerData& parseData, QString& ip);
    quint8 calcXorChecksum(const QByteArray &data, int start, int end);
    quint16 parseUInt16BE(const QByteArray &ba, int offset);
    uint32_t parseUInt32_BE(const QByteArray &ba, int offset);

    void printsData(const QString& description);

    //照明设备： 有人/无人
    bool parseSomeonePacket();
    bool parseNoonePacket();

    bool parseCNGDevice();
    bool parseJNZMDevice();
    bool parseJNKTDevice();

    static double getVoltage(uint32_t raw)   { return raw * 0.0001; }
    static double getCurrent(uint32_t raw)   { return raw * 0.0001; }
    static double getPower(uint32_t raw)     { return raw * 0.0001; }
    static double getEnergy(uint32_t raw)    { return raw * 0.0001; }
    static double getPowerFactor(uint16_t raw){ return raw * 0.001; }
    static double getCO2(uint16_t raw)       { return raw * 0.0001; }
    static double getTemperature(uint16_t raw){ return raw * 0.01; }
    static double getFrequency(uint16_t raw) { return raw * 0.01; }

    // 储能柜系统专用转换（16位电池数据）
    static double getCNG_BatteryVoltage(uint16_t raw) { return raw * 0.01; }
    static double getCNG_BatteryCurrent(uint16_t raw) { return raw * 0.01; }
    static double getCNG_BatteryTemp(uint16_t raw)    { return raw * 0.01; }

private:
    QByteArray m_buffer;
    PowerData out_pack;

    uint8_t calcXOR(const QByteArray& data, int start, int len);

    // 查找帧头位置
    int findFrameHead();

};

#endif // PACK_H
