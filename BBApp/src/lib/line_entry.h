#ifndef LINEENTRY_H
#define LINEENTRY_H

#include <QLineEdit>
#include <QWidget>

#include "../lib/bb_lib.h"

class LineEntry : public QLineEdit {
    Q_OBJECT

public:
    LineEntry(QWidget *parent);
    ~LineEntry();

    bool SetFrequency(Frequency freq);
    bool SetAmplitude(Amplitude amp);
    bool SetTime(Time t);
    bool SetValue(double val);
    bool SetValue(double val, QString &units);

    //Frequency GetFrequency() const { return frequency; }

protected:
    void mousePressEvent(QMouseEvent *);

private:
    Frequency frequency;
    Amplitude amplitude;
    Time time;
    double value;

signals:

};

#endif // LINEENTRY_H
