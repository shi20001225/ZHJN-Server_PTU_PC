#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScreen>
#include <QMouseEvent>
#include "applicationmodule.h"
#include "my_tcpsocket.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *ev)override;
    void mouseMoveEvent(QMouseEvent *ev)override;
    void mouseReleaseEvent(QMouseEvent *ev)override;

private slots:
    void on_close_clicked();

    void on_reduce_clicked();

private:
    Ui::MainWindow *ui;


    QPoint m_dragPosition;
    bool m_isDragging = false;
    ApplicationModule *ktWidget;
    ApplicationModule *gfWidget;
    ApplicationModule *zmWidget;
    My_TcpSocket *tcpserver;



};
#endif // MAINWINDOW_H
