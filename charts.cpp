#include "charts.h"
#include "ui_charts.h"

Charts::Charts(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Charts),
    mPlot(0),
    mTag1(0),
    mTag2(0)
{
    ui->setupUi(this);
    this->angleReadings = new RingBuffer;
    this->speedReadings = new RingBuffer;

    mPlot = ui->customPlot;
    // configure plot to have two right axes:
    mPlot->yAxis->setTickLabels(false);
    connect(mPlot->yAxis2, SIGNAL(rangeChanged(QCPRange)), mPlot->yAxis, SLOT(setRange(QCPRange))); // left axis only mirrors inner right axis
    mPlot->yAxis2->setVisible(true);
    mPlot->yAxis->setVisible(false);
    mPlot->axisRect()->addAxis(QCPAxis::atRight);
    mPlot->axisRect()->axis(QCPAxis::atRight, 0)->setPadding(30); // add some padding to have space for tags
    mPlot->axisRect()->axis(QCPAxis::atRight, 1)->setPadding(30);
    this->mPlot->axisRect()->setRangeDragAxes(mPlot->xAxis, mPlot->yAxis2);

    // create graphs:
    mGraph1 = mPlot->addGraph(mPlot->xAxis, mPlot->axisRect()->axis(QCPAxis::atRight, 0));
    mGraph2 = mPlot->addGraph(mPlot->xAxis, mPlot->axisRect()->axis(QCPAxis::atRight, 1));
    mGraph1->setPen(QPen(QColor(250, 120, 0)));
    mGraph2->setPen(QPen(QColor(0, 180, 60)));

    // create tags with newly introduced AxisTag class (see axistag.h/.cpp):
    mTag1 = new AxisTag(mGraph1->valueAxis());
    mTag1->setPen(mGraph1->pen());
    mTag2 = new AxisTag(mGraph2->valueAxis());
    mTag2->setPen(mGraph2->pen());

    mPlot->axisRect()->setRangeDrag(Qt::Horizontal);
    this->timer = new QTimer();

    connect(this->timer, SIGNAL(timeout()), this, SLOT(timeoutSlot()));

    this->isAcquisitionStarted = false;
    this->delay = 0;

}

Charts::~Charts()
{
    delete ui;
    delete timer;
    delete angleReadings;
    delete speedReadings;
}

void Charts::updateAngleSeries(double val)
{
    if(this->angleReadings->RB_Write(val) == false)
        this->stop();
}

void Charts::updateSpeedSeries(double val)
{
    if(this->speedReadings->RB_Write(val) == false)
        this->stop();
}

void Charts::timeoutSlot()
{
    if((this->angleReadings->isReadable() == true) && this->speedReadings->isReadable())
    {
        double value;
        this->angleReadings->RB_Read(&value);
        mGraph1->addData(0.0192*mGraph1->dataCount(), value);
        this->speedReadings->RB_Read(&value);
        mGraph2->addData(0.0192*mGraph2->dataCount(), value);

        // make key axis range scroll with the data:
        mPlot->xAxis->rescale();
        mGraph1->rescaleValueAxis(false, true);
        mGraph2->rescaleValueAxis(false, true);
        mPlot->xAxis->setRange(mPlot->xAxis->range().upper, 1, Qt::AlignRight);

        // update the vertical axis tag positions and texts to match the rightmost data point of the graphs:
        double graph1Value = mGraph1->dataMainValue(mGraph1->dataCount()-1);
        double graph2Value = mGraph2->dataMainValue(mGraph2->dataCount()-1);
        mTag1->updatePosition(graph1Value);
        mTag2->updatePosition(graph2Value);
        mTag1->setText(QString::number(graph1Value, 'f', 2));
        mTag2->setText(QString::number(graph2Value, 'f', 2));

        mPlot->replot();
    }
}


void Charts::on_pushButtonStartMonitoring_clicked()
{
    if(!this->mGraph1.isNull())
    {
        this->angleReadings->clearBuffer();
        this->mGraph1->data()->clear();
        this->speedReadings->clearBuffer();
        this->mGraph2->data()->clear();
    }
    this->isAcquisitionStarted = true;
    this->timer->start(15);
    ui->pushButtonStopMonitoring->setEnabled(true);
    ui->pushButtonStartMonitoring->setEnabled(false);
    this->mPlot->setInteraction(QCP::iRangeDrag, false);
    this->mPlot->setInteraction(QCP::iRangeZoom, false);
}


void Charts::on_pushButtonStopMonitoring_clicked()
{
    this->stop();
}

void Charts::stop()
{
    this->isAcquisitionStarted = false;
    this->timer->stop();
    this->delay = 0;

    ui->pushButtonStartMonitoring->setEnabled(true);
    ui->pushButtonStopMonitoring->setEnabled(false);

    this->mPlot->setInteraction(QCP::iRangeDrag, true);
    this->mPlot->setInteraction(QCP::iRangeZoom, true);
}

RingBuffer::RingBuffer()
{
    this->Head = 0;
    this->Tail = 0;
}

bool RingBuffer::RB_Read(double *Value)
{
    if(Tail == Head) return false;

    *Value = Buffer[Tail];
    Tail = (Tail + 1) % RING_BUFFER_SIZE;
    return true;
}

bool RingBuffer::RB_Write(double Value)
{
    int tmpHead = (Head + 1) % RING_BUFFER_SIZE;

    if(tmpHead == Tail) return false;

    Buffer[Head] = Value;
    Head = tmpHead;
    return true;
}
