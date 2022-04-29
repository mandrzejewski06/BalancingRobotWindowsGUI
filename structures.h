#ifndef STRUCTURES_H
#define STRUCTURES_H

class PID
{
public:
    PID() {}

    void setKp(double kp) { this->Kp = kp; }
    void setKi(double ki) { this->Ki = ki; }
    void setKd(double kd) { this->Kd = kd; }

    inline double getKp() const { return this->Kp; }
    inline double getKi() const { return this->Ki; }
    inline double getKd() const { return this->Kd; }
private:
    double Kp;
    double Ki;
    double Kd;
};

class IMU
{
public:
    IMU() {}

    void setGyroX_Offset(double gx) { this->GyroX_Offset = gx; }
    void setAccy_Offset(double ay)  { this->AccY_Offset = ay; }
    void setAccZ_Offset(double az)  { this->AccZ_Offset = az; }

    inline double getGyroX_Offset() const  { return this->GyroX_Offset; }
    inline double getAccY_Offset()  const  { return this->AccY_Offset; }
    inline double getAccZ_Offset()  const  { return this->AccZ_Offset; }
private:
    double GyroX_Offset;
    double AccY_Offset;
    double AccZ_Offset;
};

class Motors {
public:
    Motors() {}

    void setMicrostep(int microstep)  { this->Microstep = microstep; }
    void setMaxFrequency(int maxFreq) { this->MaxFrequency = maxFreq; }
    void setMaxSpeed(int maxSpeed)    { this->MaxSpeed = maxSpeed; }

    inline int getMicrostep()    const { return this->Microstep; }
    inline int getMaxFrequency() const { return this->MaxFrequency; }
    inline int getMaxSpeed()     const { return this->MaxSpeed; }
private:
    int Microstep;
    int MaxFrequency;
    int MaxSpeed;
};

#endif // STRUCTURES_H
