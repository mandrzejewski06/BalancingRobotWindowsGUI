#ifndef VARIABLES_H
#define VARIABLES_H

#include <QObject>
#include <QTimer>
#include "structures.h"

#define READINGS_BUFFER 1500


class Variables : public QObject
{
    Q_OBJECT

public:
    explicit Variables(QObject *parent = nullptr);
    ~Variables();

    inline void setBatteryMinVoltage(double mv) { this->batteryMinVoltage = mv; }
    inline void setBatteryMaxVoltage(double mv) { this->batteryMaxVoltage = mv; }
    inline double getBatteryMinVoltage() { return this->batteryMinVoltage; }
    inline double getBatteryMaxVoltage() { return this->batteryMaxVoltage; }

    PID *pid;
    IMU *imu;
    Motors *motors;

signals:
    void sendCommand(QString message);
    void addToLogs(QString message);

protected:
    static bool autocalibrationFinished;
    static bool wasAutocalibrationSuccessfull;

private:
    double batteryMinVoltage;
    double batteryMaxVoltage;
};

class Readings : public Variables
{
    Q_OBJECT

public:
    Readings();

    enum ReadingVars {
        // Zmienne odczytywane na żądanie
        KP, KI, KD, GX_OFF, AY_OFF, AZ_OFF, MSTEP, MAX_FREQ,
        MAX_SPD, BAT_MAX_V, BAT_MIN_V, // tu dopisywać kolejne zmienne żądane
        // Zmienne wysyłane cyklicznie przez robota
        ANGLE, ROBOT_STATUS, SPEED, BATTERY_LEVEL, // tu dopisywać kolejne zmienne cykliczne
        // Status wykonania danego algorytmu wewnątrz robota
        CALIBRATION_STATUS, // tu dopisywać kolejne algorytmy
        // Znacznik końca enum
        END_OF_ENUM
    };

    enum RobotStatus {
        STOPPED, FORWARD, BACKWARD, LEFT, RIGHT, BALANCING
    };

    ReadingVars readCommandMessage(QString line);

    void requestVar(ReadingVars option);
    void requestAll();
    void writeVar(ReadingVars var, double value);

    double getLastAngleReading();
    double getLastSpeedReading();
    void writeAngleReadings(double value);
    void writeSpeedReadings(double value);

    inline bool getLastAutocalibrationStatus() { return this->wasAutocalibrationSuccessfull; }

    inline RobotStatus getRobotStatus() { return this->robotState; }
    inline void setRobotStatus(RobotStatus mode) { this->robotState = mode; }

    inline void setBatteryLevel(int lvl) {this->batteryLevel = lvl; }
    inline int getBatteryLevel() { return this->batteryLevel; }

    inline bool isRequestingAll() { return this->isRefreshingAll; }

signals:
    void variablesLoaded();
    void newAngleReading(double val);
    void newSpeedReading(double val);

private slots:
    void timeoutOccured();
    void nextRequestTime();

private:
    double angleReadings[READINGS_BUFFER];
    double speedReadings[READINGS_BUFFER];
    int nextAngleWrite;
    int nextSpeedWrite;
    unsigned short int batteryLevel;
    unsigned short int refreshAllCounter;
    bool isRefreshingAll;


    RobotStatus robotState;

    QTimer *timerTimeout;
    QTimer *timerNextRequest;
};

class Settings : public Variables
{
    Q_OBJECT

public:
    Settings();

    enum Controls {
        FORWARD, BACKWARD, LEFT, RIGHT
    };

    enum SettingVars {
        // Zmienne odczytywane na żądanie
        KP, KI, KD, GX_OFF, AY_OFF, AZ_OFF, MSTEP, MAX_FREQ,
        MAX_SPD, BAT_MAX_V, BAT_MIN_V, // tu dopisywać kolejne nastawy
        // Znacznik końca enum
        END_OF_ENUM
    };

    void autocalibration();
    void Controls(Controls direction, bool startMotors);
    void sendSetting(SettingVars setVar, double value);
    void setMotorsBlocked(bool blocked);

signals:
    void autocalibrationDone();

private slots:
    void autocalibrationTimeout();

private:
    QTimer *timerTimeout;

};
#endif // VARIABLES_H
