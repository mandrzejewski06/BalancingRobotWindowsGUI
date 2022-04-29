#ifndef CHARTS_H
#define CHARTS_H

#include <QWidget>
#include <QElapsedTimer>
#include <QTimer>
#include "axistag.h"
#include "qcustomplot/qcustomplot.h"

#define RING_BUFFER_SIZE 500

namespace Ui {
class Charts;
}

class RingBuffer {

public:
    RingBuffer();

    inline void clearBuffer() { this->Head = this->Tail = 0; }
    inline bool isReadable() { return (this->Head == this->Tail) ? false : true; }
    bool RB_Read(double *Value);
    bool RB_Write(double Value);

    inline int getHead() { return this->Head; }
    inline int getTail() { return this->Tail; }

private:
    int Head;
    int Tail;
    double Buffer[RING_BUFFER_SIZE];
};

class Charts : public QWidget
{
    Q_OBJECT

public:
    explicit Charts(QWidget *parent = nullptr);
    ~Charts();

    void updateAngleSeries(double val);
    void updateSpeedSeries(double val);

    bool isAquisitionStarted() { return this->isAcquisitionStarted; }
public slots:
    void timeoutSlot();
private slots:
  void on_pushButtonStartMonitoring_clicked();

  void on_pushButtonStopMonitoring_clicked();

  void stop();

private:
    Ui::Charts *ui;

    QCustomPlot *mPlot;

    QPointer<QCPGraph> mGraph1;
    QPointer<QCPGraph> mGraph2;

    AxisTag *mTag1;
    AxisTag *mTag2;

    QTimer *timer;

    RingBuffer *angleReadings;
    RingBuffer *speedReadings;

    bool isAcquisitionStarted;
    int delay;
};


#endif // CHARTS_H
