#ifndef ANNUALDETAILS_H
#define ANNUALDETAILS_H

#include <QFrame>
#include <QVector>


/*
 * 具体节约月份界面
 */

namespace Ui {
class AnnualDetails;
}

class AnnualDetails : public QFrame
{
    Q_OBJECT

public:
    explicit AnnualDetails(QWidget *parent = nullptr);
    ~AnnualDetails();

    // 设置12个月减碳数据
    void setMonthlyCO2Data(const QVector<double> &monthlyCO2List);

private slots:
    void on_pushButton_clicked();

private:
    Ui::AnnualDetails *ui;
};

#endif // ANNUALDETAILS_H
