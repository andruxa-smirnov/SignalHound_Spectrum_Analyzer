#include "line_entry.h"

LineEntry::LineEntry(QWidget *parent)
    : QLineEdit(parent)
{
    setObjectName("SH_LineEntry");
    //setReadOnly(true);
    setAlignment(Qt::AlignRight);
}

LineEntry::~LineEntry()
{

}

/*
 * Set the text and update the class variable
 *  Do nothing if the values did not change
 *   Much faster to compare variables than to update the
 *    widget.
 * Returns false if the frequency is not updated
 */
bool LineEntry::SetFrequency(Frequency freq)
{
    frequency = freq;
    setText(freq.GetFreqString());

    return true;
}

bool LineEntry::SetAmplitude(Amplitude amp)
{
    amplitude = amp;
    setText(amp.GetString());
    return true;
}

bool LineEntry::SetTime(Time t)
{
    time = t;
    setText(time.GetString());
    return true;
}

bool LineEntry::SetValue(double val)
{
    if(value == val) {
        return false;
    }

    value = val;
    QString text;
    text.sprintf("%.6f", value);
    setText(text);
    return true;
}

bool LineEntry::SetValue(double val, QString &units)
{
    value = val;
    QString text;
    text.sprintf("%.6f ", value);
    text += units;
    setText(text);
    return true;
}

/*
 * Clicked signal event
 */
void LineEntry::mousePressEvent(QMouseEvent *)
{
    this->selectAll();
}
