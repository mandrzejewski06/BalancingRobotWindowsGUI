#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothSocket>
#include "variables.h"
#include <QTimer>
#include "charts.h"
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonClearLogs_clicked();

    void on_pushButtonSearchDevices_clicked();

    void on_pushButtonConnect_clicked();

    void on_pushButtonDisconnect_clicked();

    void captureDeviceProperties(const QBluetoothDeviceInfo &device);

    void searchingFinished();

    void addToLogs(QString message);

    void connectionEstablished();

    void connectionInterrupted();

    void connectionError(QBluetoothSocket::SocketError error);

    void socketReadyToRead();

    void on_pushButtonSendData_clicked();

    void on_pushButtonGoForward_pressed();

    void on_pushButtonGoForward_released();

    void on_pushButtonGoLeft_pressed();

    void on_pushButtonGoLeft_released();

    void on_pushButtonGoBackward_pressed();

    void on_pushButtonGoBackward_released();

    void on_pushButtonGoRight_pressed();

    void on_pushButtonGoRight_released();

    void on_pushButtonRefreshReadings_clicked();

    bool sendMessageToDevice(QString message);

    void refreshingDone();

    void autocalibrationDone();

    void on_pushButtonAutocalibration_clicked();

    void on_pushButtonLockMotors_clicked();

    double getSettingFromGUI(Settings::SettingVars var, bool *ok);

    void on_pushButtonSetKp_clicked();

    void on_pushButtonSetKi_clicked();

    void on_pushButtonSetKd_clicked();

    void on_pushButtonSetGxOffst_clicked();

    void on_pushButtonSetAyOffst_clicked();

    void on_pushButtonSetAzOffst_clicked();

    void on_pushButtonSetMicrostep_clicked();

    void on_pushButtonSetMaxFreq_clicked();

    void on_pushButtonSetMaxSpeed_clicked();

    void on_pushButtonSetMinVoltage_clicked();

    void on_pushButtonSetMaxVoltage_clicked();

    void on_pushButtonCharts_clicked();

private:
    Ui::MainWindow *ui;

    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QBluetoothSocket *socket;

    Readings *readings;
    Settings *settings;
    Charts *charts;

    QStringList readedData;

    //QThread blueSocketThread;
    //QThread chartsThread;

    void reloadData();
    void enableGUI(bool enable);
    void robotStatusGUI(Readings::RobotStatus status);    
};
#endif // MAINWINDOW_H
