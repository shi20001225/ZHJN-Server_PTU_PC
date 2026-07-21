#include "mainwindow.h"

#include <QApplication>
#include "logger.h"

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);

    Logger::instance()->init();
    // 注册自定义类型
    qRegisterMetaType<MonthlyData>("MonthlyData");
    qRegisterMetaType<QVector<double>>("QVector<double>");


    try {
        MainWindow w;
        w.show();
        return a.exec();
    }
    catch (const std::exception &e)
    {
        qWarning() << "" << e.what();
        return -1;
    }
    catch (...) {
        qWarning() << "未知异常";
        return -1;
    }
}
