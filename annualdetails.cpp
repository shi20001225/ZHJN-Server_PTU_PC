#include "annualdetails.h"
#include "ui_annualdetails.h"

AnnualDetails::AnnualDetails(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::AnnualDetails)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint,true);
}

AnnualDetails::~AnnualDetails()
{
    delete ui;
}

void AnnualDetails::setMonthlyCO2Data(const QVector<double> &monthlyCO2List)
{
    if (monthlyCO2List.size() != 12) {
            qDebug() << "[setMonthlyCO2Data] 数据长度错误:" << monthlyCO2List.size();
            return;
        }

        // 按索引对应月份，设置到对应的 QLabel
        ui->january->setText(QString::number(monthlyCO2List[0], 'f', 4));
        ui->february->setText(QString::number(monthlyCO2List[1], 'f', 4));
        ui->march->setText(QString::number(monthlyCO2List[2], 'f', 4));
        ui->april->setText(QString::number(monthlyCO2List[3], 'f', 4));
        ui->may->setText(QString::number(monthlyCO2List[4], 'f', 4));
        ui->june->setText(QString::number(monthlyCO2List[5], 'f', 4));
        ui->july->setText(QString::number(monthlyCO2List[6], 'f', 4));
        ui->august->setText(QString::number(monthlyCO2List[7], 'f', 4));
        ui->september->setText(QString::number(monthlyCO2List[8], 'f', 4));
        ui->october->setText(QString::number(monthlyCO2List[9], 'f', 4));
        ui->november->setText(QString::number(monthlyCO2List[10], 'f', 4));
        ui->december->setText(QString::number(monthlyCO2List[11], 'f', 4));
}

void AnnualDetails::on_pushButton_clicked()
{
    close();
}
