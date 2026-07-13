#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 注册自定义类型
    qRegisterMetaType<MonthlyData>("MonthlyData");
    qRegisterMetaType<QVector<double>>("QVector<double>");

    MainWindow w;
    w.show();
    return a.exec();
}
