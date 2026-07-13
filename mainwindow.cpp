#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint,true);
    setAttribute(Qt::WA_TranslucentBackground);
    QRect desktop = QGuiApplication::primaryScreen()->availableGeometry();
    this->move((desktop.width() - this->width())/2,(desktop.height() - this->height())/2);

    gfWidget = new ApplicationModule(CNG_D,QString("绿电储能"), ui->GF_widget, QString("产生绿电:"));
    zmWidget = new ApplicationModule(JNZM_D,QString("照明"), ui->ZM_widget);
    ktWidget = new ApplicationModule(JNKT_D,QString("空调"), ui->KT_widget);

    tcpserver = new My_TcpSocket(this);


    //QObject::connect(tcpserver, &My_TcpSocket::sig_RefreshTheInterFace_JNZM, zmWidget, &ApplicationModule::on_refreshTheInterFace);
    QObject::connect(tcpserver, &My_TcpSocket::sig_RefreshAggregatedByDeviceId, zmWidget, &ApplicationModule::on_AggregatedDataUpdated);
    QObject::connect(tcpserver, &My_TcpSocket::sig_MonthlyCO2DataUpdated, zmWidget, &ApplicationModule::on_MonthlyCO2DataUpdated);

    //QObject::connect(tcpserver, &My_TcpSocket::sig_RefreshTheInterFace_CNG, gfWidget, &ApplicationModule::on_refreshTheInterFace);
    QObject::connect(tcpserver, &My_TcpSocket::sig_RefreshAggregatedByDeviceId, gfWidget, &ApplicationModule::on_AggregatedDataUpdated);
    QObject::connect(tcpserver, &My_TcpSocket::sig_MonthlyCO2DataUpdated, gfWidget, &ApplicationModule::on_MonthlyCO2DataUpdated);

    tcpserver->refreshInitialDisplay();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *ev)
{
    //qDebug() << QString("现在点击的坐标是：X-%1   Y-%2").arg(ev->pos().rx()).arg(ev->pos().ry());
    if(ev->button() ==  Qt::LeftButton && ev->pos().ry() < 50)
    {
        m_dragPosition = ev->globalPosition().toPoint() - frameGeometry().topLeft();
        m_isDragging = true;
        ev->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *ev)
{
    if(m_isDragging && (ev->buttons() & Qt::LeftButton))
    {
        move(ev->globalPosition().toPoint() - m_dragPosition);
        ev->accept();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    if(ev->button() == Qt::LeftButton)
    {
        m_isDragging = false;
        ev->accept();
    }
}


void MainWindow::on_close_clicked()
{
    this->close();
}

void MainWindow::on_reduce_clicked()
{
    this->showMinimized();
}
