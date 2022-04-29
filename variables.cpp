//#include "mainwindow.h"
#include "variables.h"
#include <string.h>
//#include <QDebug>

#define TIMEOUT_REFRESH_ALL 1000
#define TIMEOUT_AUTOCALIBRATION 4000
#define NEXT_REQUEST_INTERVAL 100

bool Variables::autocalibrationFinished = false;
bool Variables::wasAutocalibrationSuccessfull = false;

Variables::Variables(QObject *parent)
    : QObject{parent}
{
    this->pid = new PID;
    this->imu = new IMU;
    this->motors = new Motors;

}

Variables::~Variables()
{
    delete pid;
    delete imu;
    delete motors;
}


Readings::Readings()
{
    this->nextAngleWrite = 0;
    this->nextSpeedWrite = 0;
    this->batteryLevel = 0;
    this->refreshAllCounter = 0;

    this->timerTimeout = new QTimer(this);
    this->timerTimeout->setSingleShot(true);

    this->timerNextRequest = new QTimer(this);

    connect(this->timerTimeout, &QTimer::timeout, this, &Readings::timeoutOccured);
    connect(this->timerNextRequest, &QTimer::timeout, this, &Readings::nextRequestTime);
}

Readings::ReadingVars Readings::readCommandMessage(QString line)
{
    bool ok;
    double value;
    Readings::ReadingVars var;
    QString tmp;
    QString tmpValue;
    QString terminator = "\r";
    int pos = line.lastIndexOf(terminator);
    int pos2 = pos;

    line = line.left(pos);
    //qDebug() << line;
    terminator = "=";
    pos = line.lastIndexOf(terminator);
    tmpValue = line.mid(pos+1, pos2-pos);
    tmp = line.left(pos);
    if     (tmp == "ANG")
    {
        if(tmpValue.endsWith('.') || !tmpValue.contains('.'))
            return END_OF_ENUM;
        value = tmpValue.toDouble(&ok);
        if(ok)
        {
            this->writeAngleReadings(value);
            return ANGLE;
        }
        return END_OF_ENUM;
    }
    else if(tmp == "SPD")
    {
        value = tmpValue.toDouble(&ok);
        if(ok) { this->writeSpeedReadings(value);
                  emit this->newSpeedReading(this->getLastSpeedReading()); }
        return SPEED;
    }
    else if(tmp == "ST")
    {
        int stTemp = tmpValue.toInt(&ok);
        if(ok) { this->setRobotStatus((Readings::RobotStatus) stTemp);
                  }
        return ROBOT_STATUS;
    }
    else if(tmp == "BL")
    {
        int blTemp = tmpValue.toInt(&ok);
        if (ok) { this->setBatteryLevel(blTemp); return BATTERY_LEVEL; }
        else return END_OF_ENUM;
    }
    else if(tmp == "KP")  var = Readings::KP;
    else if(tmp == "KI")  var = Readings::KI;
    else if(tmp == "KD")  var = Readings::KD;
    else if(tmp == "GX")  var = Readings::GX_OFF;
    else if(tmp == "AY")  var = Readings::AY_OFF;
    else if(tmp == "AZ")  var = Readings::AZ_OFF;
    else if(tmp == "MST") var = Readings::MSTEP;
    else if(tmp == "MF")  var = Readings::MAX_FREQ;
    else if(tmp == "MSP") var = Readings::MAX_SPD;
    else if(tmp == "BMX") var = Readings::BAT_MAX_V;
    else if(tmp == "BMN") var = Readings::BAT_MIN_V;
    else if(tmp == "CB")
    {
        int cbTemp = tmpValue.toInt(&ok);
        if(ok && cbTemp) this->wasAutocalibrationSuccessfull = true;
        else if(ok && !cbTemp) this->wasAutocalibrationSuccessfull = false;
        else return END_OF_ENUM;

        autocalibrationFinished = true;
        return CALIBRATION_STATUS;
    }
    else return END_OF_ENUM;

    value = tmpValue.toDouble(&ok);
    if(ok && !this->isRequestingAll())
    {
        this->writeVar(var, value);
        return var;
    }
    else if(ok && this->isRequestingAll())
    {
        this->timerTimeout->start(TIMEOUT_REFRESH_ALL);
        this->writeVar(var, value);
        return var;
    }
    else return END_OF_ENUM;
}

void Readings::requestVar(ReadingVars option)
{
    QString message;

    switch(option)
    {
    // Niecykliczne
    case KP:            message = "P+KP?";  break;
    case KI:            message = "P+KI?";  break;
    case KD:            message = "P+KD?";  break;
    case GX_OFF:        message = "I+GX?";  break;
    case AY_OFF:        message = "I+AY?";  break;
    case AZ_OFF:        message = "I+AZ?";  break;
    case MSTEP:         message = "M+MST?"; break;
    case MAX_FREQ:      message = "M+MF?";  break;
    case MAX_SPD:       message = "M+MSP?"; break;
    case BAT_MAX_V:     message = "O+BMX?"; break;
    case BAT_MIN_V:     message = "O+BMN?"; break;
    // Cykliczne
    case ANGLE:         message = "I+ANG?"; break;
    case SPEED:         message = "M+SPD?"; break;
    case ROBOT_STATUS:  message = "M+ST?";  break;
    case BATTERY_LEVEL: message = "O+BL?";  break;
    // Algorytmy
    case CALIBRATION_STATUS: message = "I+CB?";  break;
    default:        return;
    }

    emit this->sendCommand(message);
}

void Readings::requestAll()
{
    this->isRefreshingAll = true;
    emit this->addToLogs("Odświeżam dane...");
    this->timerNextRequest->start(NEXT_REQUEST_INTERVAL);
}

void Readings::writeVar(ReadingVars var, double value)
{
    switch(var)
    {
    case KP:        this->pid->setKp(value);  break;
    case KI:        this->pid->setKi(value);  break;
    case KD:        this->pid->setKd(value);  break;
    case GX_OFF:    this->imu->setGyroX_Offset(value);  break;
    case AY_OFF:    this->imu->setAccy_Offset(value);   break;
    case AZ_OFF:    this->imu->setAccZ_Offset(value);   break;
    case MSTEP:     this->motors->setMicrostep((int) value); break;
    case MAX_FREQ:  this->motors->setMaxFrequency((int) value);  break;
    case MAX_SPD:   this->motors->setMaxSpeed((int) value); break;
    case BAT_MAX_V: this->setBatteryMaxVoltage(value);  break;
    case BAT_MIN_V: this->setBatteryMinVoltage(value);  break;
    default: return;
    }

    if(this->isRefreshingAll)
    {
        this->refreshAllCounter--;
        if(this->refreshAllCounter == 0)
        {
            this->isRefreshingAll = false;
            this->timerTimeout->stop();
            emit this->variablesLoaded();
            emit this->addToLogs("Wszystkie ustawienia odczytane.");
        }
    }
}

double Readings::getLastAngleReading()
{
    if(this->nextAngleWrite == 0) return this->angleReadings[READINGS_BUFFER - 1];
    else return this->angleReadings[nextAngleWrite - 1];
}

double Readings::getLastSpeedReading()
{
    if(this->nextSpeedWrite == 0) return this->speedReadings[READINGS_BUFFER - 1];
    else return this->speedReadings[nextSpeedWrite - 1];
}

void Readings::writeAngleReadings(double value)
{
    this->angleReadings[this->nextAngleWrite] = value;
    this->nextAngleWrite++;
    if(this->nextAngleWrite >= READINGS_BUFFER) this->nextAngleWrite = 0;
}

void Readings::writeSpeedReadings(double value)
{
    this->speedReadings[this->nextSpeedWrite] = value;
    this->nextSpeedWrite++;
    if(this->nextSpeedWrite >= READINGS_BUFFER) this->nextSpeedWrite = 0;
}

void Readings::timeoutOccured()
{
    if(this->isRefreshingAll)
    {
        this->isRefreshingAll = false;
        emit this->addToLogs("Timeout: brakuje " + QString::number(this->refreshAllCounter) + " danych!");
        this->refreshAllCounter = 0;
        emit this->variablesLoaded();
    }
}

void Readings::nextRequestTime()
{
    static int i = 0;
    this->requestVar((ReadingVars) i);
    this->refreshAllCounter++;
    i++;

    if(i == (int) ANGLE)
    {
        this->timerNextRequest->stop();
        emit this->addToLogs("Zapytania wysłane...");
        this->timerTimeout->start(TIMEOUT_REFRESH_ALL);
        i = 0;
    }
}

Settings::Settings()
{
    this->timerTimeout = new QTimer(this);
    this->timerTimeout->setSingleShot(true);

    connect(this->timerTimeout, &QTimer::timeout, this, &Settings::autocalibrationTimeout);
}

void Settings::autocalibration()
{
    QString message = "I+CB=1";
    if(autocalibrationFinished == false)
    {
        emit this->sendCommand(message);
        this->timerTimeout->start(TIMEOUT_AUTOCALIBRATION);
    }
    else
        emit this->addToLogs("Poprzednia autokalibracja nie jest zakończona!");

}

void Settings::Controls(enum Controls direction, bool startMotors)
{
    QString message;

    switch(direction)
    {
    case FORWARD:   message = ("M+FWD=" + QString::number(startMotors)); break;
    case BACKWARD:  message = ("M+BCK=" + QString::number(startMotors)); break;
    case LEFT:      message = ("M+LFT=" + QString::number(startMotors)); break;
    case RIGHT:     message = ("M+RGT=" + QString::number(startMotors)); break;
    default: return;
    }

    emit this->sendCommand(message);
}

void Settings::sendSetting(SettingVars setVar, double value)
{
    QString message;
    QString tmpValue = QString::number((double) value, 'f', 5);

    switch (setVar)
    {
    case KP:
        this->pid->setKp(value);
        message = ("P+KP="  + tmpValue); break;
    case KI:
        this->pid->setKi(value);
        message = ("P+KI="  + tmpValue); break;
    case KD:
        this->pid->setKd(value);
        message = ("P+KD="  + tmpValue); break;
    case GX_OFF:
        this->imu->setGyroX_Offset(value);
        message = ("I+GX="  + tmpValue); break;
    case AY_OFF:
        this->imu->setAccy_Offset(value);
        message = ("I+AY="  + tmpValue); break;
    case AZ_OFF:
        this->imu->setAccZ_Offset(value);
        message = ("I+AZ="  + tmpValue); break;
    case MSTEP:
        this->motors->setMicrostep((int) value);
        message = ("M+MST=" + tmpValue.left(tmpValue.lastIndexOf("."))); break;
    case MAX_FREQ:
        this->motors->setMaxFrequency((int) value);
        message = ("M+MF="  + tmpValue.left(tmpValue.lastIndexOf("."))); break;
    case MAX_SPD:
        this->motors->setMaxSpeed((int) value);
        message = ("M+MSP=" + tmpValue.left(tmpValue.lastIndexOf("."))); break;
    case BAT_MAX_V:
        this->setBatteryMaxVoltage(value);
        message = ("O+BMX=" + tmpValue); break;
    case BAT_MIN_V:
        this->setBatteryMinVoltage(value);
        message = ("O+BMN=" + tmpValue); break;
    default: return;
    }

    emit sendCommand(message);
}

void Settings::setMotorsBlocked(bool blocked)
{
    QString message;

    if(blocked)
        message = "M+BLK=1";
    else
        message = "M+BLK=0";

    emit this->sendCommand(message);
}

void Settings::autocalibrationTimeout()
{
    if(autocalibrationFinished == false)
        emit this->addToLogs("Autokalibracja timeout!");
    if(autocalibrationFinished == true)
        autocalibrationFinished = false;

    emit autocalibrationDone();
}



