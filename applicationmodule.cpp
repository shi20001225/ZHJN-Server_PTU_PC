#include "applicationmodule.h"
#include "ui_applicationmodule.h"

ApplicationModule::ApplicationModule(int deviceID, QString applicationName, QWidget *parent, QString applicationFunction) :
    m_deviceId(deviceID),
    QFrame(parent),
    ui(new Ui::ApplicationModule)
{
    ui->setupUi(this);
    ui->moduleName->setText(applicationName);
    ui->label_3->setText(applicationFunction);
    annualDetailsWidget = new AnnualDetails();

    setupMonthlyRefresh(this, [this]() {
        auto d = QDate::currentDate();
        ui->label_month1->setText(QString("%1月份").arg(d.month()));
        ui->label_month2->setText(QString("%1月份").arg(d.month()));
        ui->label_year->setText(QString("%1年(统计)").arg(d.year()));
    });


}

ApplicationModule::~ApplicationModule()
{
    delete ui;
    delete annualDetailsWidget;
}

void ApplicationModule::setupMonthlyRefresh(QObject *parent, std::function<void ()> callback)
{
    auto timer = new QTimer(parent);
    timer->setSingleShot(true);

    auto reset = [timer]() {
        auto now = QDateTime::currentDateTime();
        auto next = QDateTime(QDate(now.date().year(), now.date().month(), 1).addMonths(1), QTime(0,0,0));
        timer->start(now.msecsTo(next));
    };

    QObject::connect(timer, &QTimer::timeout, parent, [callback, reset]() {
        callback();
        reset();
    });

    callback();
    reset();
}

void ApplicationModule::on_pushButton_clicked()
{
    annualDetailsWidget->show();
}

void ApplicationModule::on_refreshTheInterFace(const MonthlyData &monthlyData, double yearReducedCO2)
{
    ui->line_generateGreenElectricity->setText(QString::number(monthlyData.savedEnergy)) ;
    ui->line_costSavings->setText(QString::number(monthlyData.savedCost));
    ui->line_month_carbonEmissions->setText(QString::number(monthlyData.reducedCO2));
    ui->line_year_carbonEmissions->setText(QString::number(yearReducedCO2));
}

void ApplicationModule::on_AggregatedDataUpdated(int deviceId, const MonthlyData &monthlyData, double yearlyReducedCO2)
{
    if (deviceId == m_deviceId) {  // JNZM_D or JNKT_D or CNG_D
        // 月度聚合数据
        ui->line_generateGreenElectricity->setText(QString::number(monthlyData.savedEnergy, 'f', 2));
        ui->line_costSavings->setText(QString::number(monthlyData.savedCost, 'f', 2));
        ui->line_month_carbonEmissions->setText(QString::number(monthlyData.reducedCO2, 'f', 2));

        // 年度减碳排放量
        ui->line_year_carbonEmissions->setText(QString::number(yearlyReducedCO2, 'f', 2));
    }

    //qDebug() << QString("[applicationModule]显示的数据---节约用电：%1   节约成本：%2     月碳排放减少：%3    年碳排放减少：%4").arg(monthlyData.savedEnergy).arg(monthlyData.savedCost).arg(monthlyData.reducedCO2).arg(yearlyReducedCO2);
}

// 新增：接收12个月减碳数据
void ApplicationModule::on_MonthlyCO2DataUpdated(int deviceId, const QVector<double> &monthlyCO2List)
{
    if (deviceId != m_deviceId) return;

    if (monthlyCO2List.size() != 12) {
        qDebug() << "[on_MonthlyCO2DataUpdated] 数据长度错误:" << monthlyCO2List.size();
        return;
    }

    annualDetailsWidget->setMonthlyCO2Data(monthlyCO2List);
}
