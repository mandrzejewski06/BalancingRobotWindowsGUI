#ifndef AXISTAG_H
#define AXISTAG_H

#include "qcustomplot/qcustomplot.h"
#include <QObject>

class AxisTag : public QObject
{
    Q_OBJECT
public:
    explicit AxisTag(QCPAxis *parentAxis);

    virtual ~AxisTag();

      // setters:
      void setPen(const QPen &pen);
      void setBrush(const QBrush &brush);
      void setText(const QString &text);

      // getters:
      inline QPen pen() const { return mLabel->pen(); }
      inline QBrush brush() const { return mLabel->brush(); }
      inline QString text() const { return mLabel->text(); }

      // other methods:
      void updatePosition(double value);

    protected:
      QCPAxis *mAxis;
      QPointer<QCPItemTracer> mDummyTracer;
      QPointer<QCPItemLine> mArrow;
      QPointer<QCPItemText> mLabel;
};

#endif // AXISTAG_H
