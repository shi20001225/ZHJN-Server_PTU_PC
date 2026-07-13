#ifndef APPLICATIONMODULE_H
#define APPLICATIONMODULE_H

#include <QFrame>
#include <QVector>

#include "annualdetails.h"
#include "monthlydatastore.h"

namespace Ui {
class ApplicationModule;
}

class ApplicationModule : public QFrame
{
    Q_OBJECT

public:
    explicit ApplicationModule(int deviceId, QString applicationName, QWidget *parent = nullptr, QString applicationFunction = "节约用电:");
    ~ApplicationModule();

private slots:
    void on_pushButton_clicked();

public slots:
    void on_refreshTheInterFace(const MonthlyData &monthlyData,  double yearData);
    // 接受年和月的信息
    void on_AggregatedDataUpdated(int deviceId, const MonthlyData &data, double yearlyReducedCO2);
    // 接收12个月减碳数据
    void on_MonthlyCO2DataUpdated(int deviceId, const QVector<double> &monthlyCO2List);

private:
    Ui::ApplicationModule *ui;
    AnnualDetails *annualDetailsWidget;
    int m_deviceId;

};

#endif // APPLICATIONMODULE_H
