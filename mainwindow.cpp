#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    this->socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    this->readings = new Readings;
    this->settings = new Settings;
    this->charts = new Charts;

    //this->charts->moveToThread(&chartsThread);
    //this->socket->moveToThread(&blueSocketThread);

    //connect(&blueSocketThread, &QThread::finished, this->socket, &QObject::deleteLater, Qt::ConnectionType::DirectConnection);
    //connect(&chartsThread, &QThread::finished, this->charts, &QObject::deleteLater, Qt::ConnectionType::DirectConnection);

    connect(this->discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(captureDeviceProperties(QBluetoothDeviceInfo)));
    connect(this->discoveryAgent, SIGNAL(finished()), this, SLOT(searchingFinished()));

    connect(this->socket, SIGNAL(connected()), this, SLOT(connectionEstablished()), Qt::ConnectionType::QueuedConnection);
    connect(this->socket, SIGNAL(disconnected()), this, SLOT(connectionInterrupted()), Qt::ConnectionType::QueuedConnection);
    connect(this->socket, SIGNAL(readyRead()), this, SLOT(socketReadyToRead()), Qt::ConnectionType::QueuedConnection);
    connect(this->socket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error), this,
        [=](QBluetoothSocket::SocketError error){this->connectionError(error);}, Qt::ConnectionType::QueuedConnection);

    connect(this->readings, &Readings::addToLogs, this, &MainWindow::addToLogs);
    connect(this->readings, SIGNAL(sendCommand(QString)), this, SLOT(sendMessageToDevice(QString)));
    connect(this->readings, &Readings::variablesLoaded, this, &MainWindow::refreshingDone);

    connect(this->settings, &Settings::addToLogs, this, &MainWindow::addToLogs);
    connect(this->settings, SIGNAL(sendCommand(QString)), this, SLOT(sendMessageToDevice(QString)));
    connect(this->settings, SIGNAL(autocalibrationDone()), this, SLOT(autocalibrationDone()));

    //connect(this->readings, SIGNAL(newAngleReading(double)), this->charts, SLOT(updateAngleSeries(double)));

    //blueSocketThread.start();
    //chartsThread.start();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete readings;
    delete settings;

}

void MainWindow::on_pushButtonClearLogs_clicked()
{
    ui->textEditLogs->clear();
}


void MainWindow::on_pushButtonSearchDevices_clicked()
{
    this->addToLogs("Szukanie...");

    ui->comboBoxDevices->clear();
    ui->pushButtonSearchDevices->setEnabled(false);
    ui->pushButtonConnect->setEnabled(false);
    ui->pushButtonDisconnect->setEnabled(false);
    this->discoveryAgent->start();  
}

void MainWindow::on_pushButtonConnect_clicked()
{
    QString comboBoxQString = ui->comboBoxDevices->currentText();
    QStringList portList = comboBoxQString.split(" ");
    QString deviceAddres = portList.last();

    ui->pushButtonConnect->setEnabled(false);
    ui->pushButtonDisconnect->setEnabled(false);

    static const QString serviceUuid(QStringLiteral("00001101-0000-1000-8000-00805F9B34FB"));
    this->socket->connectToService(QBluetoothAddress(deviceAddres),QBluetoothUuid(serviceUuid),QIODevice::ReadWrite);
    this->addToLogs("Rozpoczęto łączenie z urządzeniem o nazwie: " + portList.first() + " i adresie: " + deviceAddres);
}


void MainWindow::on_pushButtonDisconnect_clicked()
{
    ui->pushButtonConnect->setEnabled(false);
    ui->pushButtonDisconnect->setEnabled(false);

    this->addToLogs("Zamykam połączenie.");
    this->socket->disconnectFromService();
}

void MainWindow::captureDeviceProperties(const QBluetoothDeviceInfo &device)
{
    ui->comboBoxDevices->addItem(device.name() + " " + device.address().toString());
    this->addToLogs("Znaleziono urządzenie o nazwie: " + device.name() + " i adresie: " + device.address().toString());
}

void MainWindow::searchingFinished()
{
    ui->pushButtonSearchDevices->setEnabled(true);
    ui->pushButtonConnect->setEnabled(true);
}

void MainWindow::addToLogs(QString message)
{
    QString currentDateTime = QDateTime::currentDateTime().toString("hh:mm:ss");
    ui->textEditLogs->append(currentDateTime + "\t" + message);
}

void MainWindow::connectionEstablished()
{
    this->addToLogs("Połączenie ustanowione.");
    this->reloadData();
    ui->pushButtonDisconnect->setEnabled(true);
    ui->pushButtonConnect->setEnabled(false);
}

void MainWindow::connectionInterrupted()
{
    this->addToLogs("Połączenie przerwane...");
    this->socket->reset();
    ui->pushButtonDisconnect->setEnabled(false);
    ui->pushButtonConnect->setEnabled(true);
}

void MainWindow::connectionError(QBluetoothSocket::SocketError error)
{
    static QBluetoothSocket::SocketError lastError = QBluetoothSocket::NoSocketError;

    if(lastError != error)
    {
        this->addToLogs("Error podczas łączenia: " + QString::number(error));
        ui->pushButtonConnect->setEnabled(true);
        lastError = error;

        this->socket->reset();
    }
}

void MainWindow::socketReadyToRead()
{
    QString readedData = 0;
    QStringList readedCommands;
    if(this->socket->bytesAvailable())
    {

       int cmdNumber;
       while(this->socket->canReadLine())
           readedData += this->socket->readLine();

       //readedData = this->socket->readAll();
       readedCommands = readedData.split('\n');
       //readedCommands.removeDuplicates();
       cmdNumber = readedCommands.size();
       for(int i = 0; i<cmdNumber; i++)
       {
           readedData = readedCommands[i];

           double tmpdouble;
           int tmpInt;

           Readings::ReadingVars cmd = this->readings->readCommandMessage(readedData);

           switch (cmd)
           {
           case Readings::KP:
              tmpdouble = this->readings->pid->getKp();
              ui->lineEditSetKp->setText(QString::number((double) tmpdouble, 'f', 3));
              ui->lineEditKp->setText(QString::number((double) tmpdouble, 'f', 3));
              break;
           case Readings::KI:
              tmpdouble = this->readings->pid->getKi();
              ui->lineEditSetKi->setText(QString::number((double) tmpdouble, 'f', 3));
              ui->lineEditKi->setText(QString::number((double) tmpdouble, 'f', 3));
              break;
           case Readings::KD:
              tmpdouble = this->readings->pid->getKd();
              ui->lineEditSetKd->setText(QString::number((double) tmpdouble, 'f', 3));
              ui->lineEditKd->setText(QString::number((double) tmpdouble, 'f', 3));
              break;
           case Readings::GX_OFF:
              tmpdouble = this->readings->imu->getGyroX_Offset();
              ui->lineEditSetGxOffst->setText(QString::number((double) tmpdouble, 'f', 3));
              ui->lineEditGxOffst->setText(QString::number((double) tmpdouble, 'f', 3));
              break;
           case Readings::AY_OFF:
              tmpdouble = this->readings->imu->getAccY_Offset();
              ui->lineEditSetAyOffst->setText(QString::number((double) tmpdouble, 'f', 3));
              ui->lineEditAyOffst->setText(QString::number((double) tmpdouble, 'f', 3));
              break;
           case Readings::AZ_OFF:
              tmpdouble = this->readings->imu->getAccZ_Offset();
              ui->lineEditSetAzOffst->setText(QString::number((double) tmpdouble, 'f', 3));
              ui->lineEditAzOffst->setText(QString::number((double) tmpdouble, 'f', 3));
              break;
           case Readings::MSTEP:
              tmpInt = this->readings->motors->getMicrostep();
              ui->lineEditSetMicrostep->setText(QString::number(tmpInt));
              ui->lineEditMicrostep->setText(QString::number(tmpInt));
              break;
           case Readings::MAX_FREQ:
              tmpInt = this->readings->motors->getMaxFrequency();
               ui->lineEditSetMaxFreq->setText(QString::number(tmpInt));
               ui->lineEditMaxFreq->setText(QString::number(tmpInt));
               break;
            case Readings::MAX_SPD:
               tmpInt = this->readings->motors->getMaxSpeed();
               ui->lineEditSetMaxSpeed->setText(QString::number(tmpInt));
               ui->lineEditMaxSpeed->setText(QString::number(tmpInt));
               break;
            case Readings::BAT_MAX_V:
               tmpdouble = this->readings->getBatteryMaxVoltage();
               ui->lineEditSetMaxVoltage->setText(QString::number((double) tmpdouble, 'f', 3));
               break;
            case Readings::BAT_MIN_V:
               tmpdouble = this->readings->getBatteryMinVoltage();
               ui->lineEditSetMinVoltage->setText(QString::number((double) tmpdouble, 'f', 3));
               break;
            // Cykliczne
            case Readings::ANGLE:
               tmpdouble = this->readings->getLastAngleReading();
               if(this->charts->isAquisitionStarted())
                    this->charts->updateAngleSeries(tmpdouble);
               ui->lineEditCurrentAngle->setText(QString::number((double) tmpdouble, 'f', 2));
               //this->addToLogs(QString::number(tmpdouble, 'f', 3));
               break;
            case Readings::SPEED:
               tmpInt = this->readings->getLastSpeedReading();
               if(this->charts->isAquisitionStarted())
                   this->charts->updateSpeedSeries(tmpInt);
               ui->lineEditVelocity->setText(QString::number(tmpInt));
               break;
            case Readings::ROBOT_STATUS:
               robotStatusGUI(this->readings->getRobotStatus());
               break;
            case Readings::BATTERY_LEVEL:
               tmpInt = this->readings->getBatteryLevel();
               ui->progressBarBattery->setValue(tmpInt);
               break;
            // Algorytmy
            case Readings::CALIBRATION_STATUS:
               if(this->readings->getLastAutocalibrationStatus())
                    addToLogs("Autokalibracja zakończona sukcesem.");
               else
                   addToLogs("Autokalibracja zakończona niepowodzeniem.");

               autocalibrationDone();
               break;
            default: continue;
            }
       }
    }
}

bool MainWindow::sendMessageToDevice(QString message)
{
    if(this->socket->isOpen() && this->socket->isWritable())
    {
        message+= "\n";
        this->socket->write(message.toStdString().c_str());
        return true;
    }
    else
    {
        this->addToLogs("Nie mogę wysłać wiadomości. Połączenie nie zostało ustanowione!");
        return false;
    }
}

void MainWindow::refreshingDone()
{
    this->enableGUI(true);
}

void MainWindow::autocalibrationDone()
{
    this->readings->requestVar(Readings::GX_OFF);
    this->readings->requestVar(Readings::AY_OFF);
    this->readings->requestVar(Readings::AZ_OFF);

    this->enableGUI(true);
}

void MainWindow::reloadData()
{
    this->enableGUI(false);
    this->readings->requestAll();
}

void MainWindow::enableGUI(bool enable)
{
    ui->groupBoxReadings->setEnabled(enable);
    ui->groupBoxSettings->setEnabled(enable);
    ui->groupBoxSendData->setEnabled(enable);
    ui->groupBoxControls->setEnabled(enable);
    ui->groupBoxReadings->setEnabled(enable);
}

void MainWindow::robotStatusGUI(Readings::RobotStatus status)
{
    QString text;

    switch(status)
    {
    case Readings::STOPPED:     text = "STOP";      break;
    case Readings::FORWARD:     text = "PROSTO";    break;
    case Readings::BACKWARD:    text = "WSTECZ";    break;
    case Readings::LEFT:        text = "LEWO";      break;
    case Readings::RIGHT:       text = "PRAWO";     break;
    case Readings::BALANCING:   text = "BALANS";    break;
    default: return;
    }

    ui->lineEditMotorsStatus->setText(text);
}

void MainWindow::on_pushButtonSendData_clicked()
{
    QString message = ui->lineEditSendData->text();
    qDebug() << message;
    if(sendMessageToDevice(message))
    {
        ui->lineEditSendData->clear();
    }
}

void MainWindow::on_pushButtonGoForward_pressed()
{
    this->settings->Controls(Settings::FORWARD, true);
}

void MainWindow::on_pushButtonGoForward_released()
{
    this->settings->Controls(Settings::FORWARD, false);
}

void MainWindow::on_pushButtonGoLeft_pressed()
{
    this->settings->Controls(Settings::LEFT, true);
}

void MainWindow::on_pushButtonGoLeft_released()
{
    this->settings->Controls(Settings::LEFT, false);
}

void MainWindow::on_pushButtonGoBackward_pressed()
{
    this->settings->Controls(Settings::BACKWARD, true);
}

void MainWindow::on_pushButtonGoBackward_released()
{
    this->settings->Controls(Settings::BACKWARD, false);
}

void MainWindow::on_pushButtonGoRight_pressed()
{
    this->settings->Controls(Settings::RIGHT, true);
}

void MainWindow::on_pushButtonGoRight_released()
{
    this->settings->Controls(Settings::RIGHT, false);
}


void MainWindow::on_pushButtonRefreshReadings_clicked()
{
    if(this->socket->isOpen())
        this->reloadData();
    else
        this->addToLogs("Brak połączenia!");
}


void MainWindow::on_pushButtonAutocalibration_clicked()
{
    this->enableGUI(false);
    this->settings->autocalibration();
}


void MainWindow::on_pushButtonLockMotors_clicked()
{
    static bool state = false;
    if(!state)
    {
        state = true;
        this->settings->setMotorsBlocked(state);
        ui->pushButtonLockMotors->setText("Odblokuj silniki");
    }
    else
    {
        state = false;
        this->settings->setMotorsBlocked(state);
        ui->pushButtonLockMotors->setText("Zablokuj silniki");
    }
}

double MainWindow::getSettingFromGUI(Settings::SettingVars var, bool *ok)
{
    double tmp;

    switch(var)
    {
    case Settings::KP:
        tmp = ui->lineEditSetKp->text().toDouble(ok); break;
    case Settings::KI:
        tmp = ui->lineEditSetKi->text().toDouble(ok); break;
    case Settings::KD:
        tmp = ui->lineEditSetKd->text().toDouble(ok); break;
    case Settings::GX_OFF:
        tmp = ui->lineEditSetGxOffst->text().toDouble(ok); break;
    case Settings::AY_OFF:
        tmp = ui->lineEditSetAyOffst->text().toDouble(ok); break;
    case Settings::AZ_OFF:
        tmp = ui->lineEditSetAzOffst->text().toDouble(ok); break;
    case Settings::MSTEP:
        tmp = ui->lineEditSetMicrostep->text().toDouble(ok); break;
    case Settings::MAX_FREQ:
        tmp = ui->lineEditSetMaxFreq->text().toDouble(ok); break;
    case Settings::MAX_SPD:
        tmp = ui->lineEditSetMaxSpeed->text().toDouble(ok); break;
    case Settings::BAT_MAX_V:
        tmp = ui->lineEditSetMaxVoltage->text().toDouble(ok); break;
    case Settings::BAT_MIN_V:
        tmp = ui->lineEditSetMinVoltage->text().toDouble(ok); break;
    default: *ok = false; tmp = 0;
    }

    return tmp;
}

void MainWindow::on_pushButtonSetKp_clicked()
{
    bool ok = true;
    double value = this->getSettingFromGUI(Settings::KP, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::KP, value);
        this->readings->requestVar(Readings::KP);
    }
    else this->addToLogs("Błędna wartość!");
}

void MainWindow::on_pushButtonSetKi_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::KI, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::KI, value);
        this->readings->requestVar(Readings::KI);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonSetKd_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::KD, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::KD, value);
        this->readings->requestVar(Readings::KD);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonSetGxOffst_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::GX_OFF, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::GX_OFF, value);
        this->readings->requestVar(Readings::GX_OFF);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonSetAyOffst_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::AY_OFF, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::AY_OFF, value);
        this->readings->requestVar(Readings::AY_OFF);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonSetAzOffst_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::AZ_OFF, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::AZ_OFF, value);
        this->readings->requestVar(Readings::AZ_OFF);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonSetMicrostep_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::MSTEP, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::MSTEP, value);
        this->readings->requestVar(Readings::MSTEP);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonSetMaxFreq_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::MAX_FREQ, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::MAX_FREQ, value);
        this->readings->requestVar(Readings::MAX_FREQ);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonSetMaxSpeed_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::MAX_SPD, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::MAX_SPD, value);
        this->readings->requestVar(Readings::MAX_SPD);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonSetMinVoltage_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::BAT_MIN_V, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::BAT_MIN_V, value);
        this->readings->requestVar(Readings::BAT_MIN_V);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonSetMaxVoltage_clicked()
{
    bool ok;
    double value = this->getSettingFromGUI(Settings::BAT_MAX_V, &ok);
    if(ok)
    {
        this->settings->sendSetting(Settings::BAT_MAX_V, value);
        this->readings->requestVar(Readings::BAT_MAX_V);
    }
    else this->addToLogs("Błędna wartość!");
}


void MainWindow::on_pushButtonCharts_clicked()
{
    this->charts->show();
}

