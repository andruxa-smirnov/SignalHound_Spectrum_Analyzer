#include "../lib/bb_lib.h"
#include "entry_widgets.h"

#include <QLabel>
#include <QComboBox>
#include <QColorDialog>
#include <QFileDialog>

static const int MIN_ENTRY_SIZE = 120;
static const int MAX_ENTRY_SIZE = 200;

LineEntry::LineEntry(EntryType type, QWidget *parent)
    : entry_type(type),
      QLineEdit(parent)
{
    setObjectName("SH_LineEntry");
    setAlignment(Qt::AlignRight);

    if(entry_type == FREQ_ENTRY) {
        setToolTip(tr("e.g. 20MHz = 20m or 20M or 20 mhz"));
    }

    connect(this, SIGNAL(editingFinished()),
            this, SLOT(editChanged()));
}

// Set the text and update the class variable
//  Do nothing if the values did not change
//   Much faster to compare variables than to update the
//    widget.
// Returns false if the frequency is not updated
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
    value = val;
    QString text;
    text.sprintf("%.3f", value);
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

void LineEntry::editChanged()
{
    if(entry_type == FREQ_ENTRY) {
        Frequency f = frequency;
        // If not valid frequency, reset entry
        bool valid_freq = Frequency::IsValidFreqString(text(), f);
        if(!valid_freq || f == frequency) {
            SetFrequency(frequency);
            return;
        }
        // Set new frequency, emit signal if different
        SetFrequency(f);
        emit entryUpdated();
    } else if(entry_type == AMP_ENTRY) {

    } else if(entry_type == TIME_ENTRY) {

    } else if(entry_type == VALUE_ENTRY) {
        bool valid;
        double d = text().toDouble(&valid);
        if(!valid || d == value) {
            SetValue(value);
            return;
        }
        // Set new value, emit signal
        SetValue(d);
        emit entryUpdated();
    }
}

// Frequency entry widget
FrequencyEntry::FrequencyEntry(const QString &label_text,
                               Frequency f,
                               QWidget *parent)
    : QWidget(parent),
      freq(f)
{
    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    label = new Label(label_text, this);
    entry = new LineEntry(FREQ_ENTRY, this);

    entry->SetFrequency(freq);

    connect(entry, SIGNAL(entryUpdated()), this, SLOT(editUpdated()));
}

void FrequencyEntry::SetFrequency(const Frequency &f)
{
    if(f == freq) {
        return;
    }

    freq = f;
    entry->SetFrequency(freq);
}

void FrequencyEntry::resizeEvent(QResizeEvent *)
{
    int entryWidth = (width() < 200) ? 120 : qMin(width() - 80, 210);
    int lblWidth = qMax(200, width()) - entryWidth;

    label->move(ENTRY_OFFSET, 0);
    label->resize(lblWidth - ENTRY_OFFSET, ENTRY_H);
    entry->move(lblWidth, 0);
    entry->resize(entryWidth, ENTRY_H);
}

void FrequencyEntry::editUpdated()
{
    freq = entry->GetFrequency();
    emit freqViewChanged(freq);
}

// Frequency Shift Entry Widget
FreqShiftEntry::FreqShiftEntry(const QString &label_text,
                               Frequency f,
                               QWidget *parent) :
    QWidget(parent)
{
    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    freq = f;

    label = new Label(label_text, this);

    up_btn = new QPushButton(this); //QIcon(":icons/plus.png"), "", this);
    up_btn->setObjectName("Increment");
    up_btn->resize(25, ENTRY_H);

    down_btn = new QPushButton(this); //QIcon(":icons/minus.png"), "", this);
    down_btn->setObjectName("Decrement");
    down_btn->resize(25, ENTRY_H);

    entry = new LineEntry(FREQ_ENTRY, this);
    entry->SetFrequency(freq);

    connect(up_btn, SIGNAL(clicked()), this, SLOT(clickedUp()));
    connect(down_btn, SIGNAL(clicked()), this, SLOT(clickedDown()));
    connect(entry, SIGNAL(entryUpdated()),  this, SLOT(editUpdated()));

}

void FreqShiftEntry::SetFrequency(Frequency f)
{
    if(f == freq) {
        return;
    }

    freq = f;
    entry->SetFrequency(freq);
}

void FreqShiftEntry::resizeEvent(QResizeEvent *)
{
    int entryWidth = (width() < 200) ? 120 : qMin(width() - 80, 210);
    int lblWidth = qMax(200, width()) - entryWidth;

    label->move(ENTRY_OFFSET, 0);
    label->resize(lblWidth - ENTRY_OFFSET, ENTRY_H);
    up_btn->move(lblWidth, 0);
    down_btn->move(lblWidth + 25, 0);
    entry->move(lblWidth + 50, 0);
    entry->resize(entryWidth - 50, ENTRY_H);
}

void FreqShiftEntry::editUpdated()
{
    freq = entry->GetFrequency();
    emit freqViewChanged(freq);
}

// Amplitude Panel Entry widget, no shifts
AmpEntry::AmpEntry(const QString &label_text,
                   Amplitude a,
                   QWidget *parent) :
    QWidget(parent)
{
    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    amplitude = a;

    label = new Label(label_text, this);
    entry = new LineEntry(VALUE_ENTRY, this);
    units = new ComboBox(this);
    units->resize(60, ENTRY_H);

    QStringList unit_list;
    unit_list << "dBm" << "dBmV" << "dBuV" << "mV";
    units->insertItems(0, unit_list);
    last_unit_index = 0;

    SetAmplitude(amplitude);

    connect(entry, SIGNAL(entryUpdated()), this, SLOT(editUpdated()));
    connect(units, SIGNAL(activated(int)), this, SLOT(unitsUpdated(int)));
}

void AmpEntry::SetAmplitude(Amplitude a)
{
    amplitude = a;
    entry->SetValue(amplitude.Val());
    units->setCurrentIndex(amplitude.Units());
}

void AmpEntry::resizeEvent(QResizeEvent *)
{
    int entryWidth = (width() < 200) ? 120 : qMin(width() - 80, 210);
    int lblWidth = qMax(200, width()) - entryWidth;

    label->move(ENTRY_OFFSET, 0);
    label->resize(lblWidth - ENTRY_OFFSET, ENTRY_H);
    entry->move(lblWidth, 0);
    entry->resize(entryWidth - 60, ENTRY_H);
    units->move(width() - units->width(), 0);
}

void AmpEntry::editUpdated()
{
    emit amplitudeChanged(Amplitude(entry->GetValue(), (AmpUnits)units->currentIndex()));
}

// When the units change, update the associated value to
//   the new unit type
void AmpEntry::unitsUpdated(int)
{
    if(units->currentIndex() != last_unit_index) {
        entry->SetValue(unit_convert(entry->GetValue(), (AmpUnits)last_unit_index,
                                     (AmpUnits)units->currentIndex()));
        last_unit_index = units->currentIndex();
    }
    emit amplitudeChanged(Amplitude(entry->GetValue(), (AmpUnits)units->currentIndex()));
}

// Amplitude Panel Entry widget
AmplitudeEntry::AmplitudeEntry(const QString &label_text,
                               Amplitude a, QWidget *parent) :
    QWidget(parent)
{
    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    amplitude = a;

    label = new Label(label_text, this);

    up_btn = new QPushButton(this);
    up_btn->setObjectName("Increment");
    up_btn->resize(25, ENTRY_H);

    down_btn = new QPushButton(this);
    down_btn->setObjectName("Decrement");
    down_btn->resize(25, ENTRY_H);

    entry = new LineEntry(VALUE_ENTRY, this);

    units = new ComboBox(this);
    units->resize(60, ENTRY_H);

    QStringList unit_list;
    unit_list << "dBm" << "dBmV" << "dBuV" << "mV";
    units->insertItems(0, unit_list);
    last_unit_index = 0;

    SetAmplitude(amplitude);

    connect(up_btn, SIGNAL(clicked()), this, SLOT(clickedUp()));
    connect(down_btn, SIGNAL(clicked()), this, SLOT(clickedDown()));
    connect(entry, SIGNAL(entryUpdated()), this, SLOT(editUpdated()));
    connect(units, SIGNAL(activated(int)), this, SLOT(unitsUpdated(int)));
}

void AmplitudeEntry::SetAmplitude(Amplitude a)
{
    amplitude = a;
    entry->SetValue(amplitude.Val());
    units->setCurrentIndex(amplitude.Units());
}

void AmplitudeEntry::resizeEvent(QResizeEvent *)
{
    int entryWidth = (width() < 200) ? 120 : qMin(width() - 80, 210);
    int lblWidth = qMax(200, width()) - entryWidth;

    label->move(ENTRY_OFFSET, 0);
    label->resize(lblWidth - ENTRY_OFFSET, ENTRY_H);
    up_btn->move(lblWidth, 0);
    down_btn->move(lblWidth + 25, 0);
    entry->move(lblWidth + 50, 0);
    entry->resize(entryWidth - 110, ENTRY_H);
    units->move(width() - units->width(), 0);
}

void AmplitudeEntry::editUpdated()
{
    emit amplitudeChanged(Amplitude(entry->GetValue(), (AmpUnits)units->currentIndex()));
}

// When the units change, update the associated value to
//   the new unit type
void AmplitudeEntry::unitsUpdated(int)
{
    if(units->currentIndex() != last_unit_index) {
        entry->SetValue(unit_convert(entry->GetValue(), (AmpUnits)last_unit_index,
                                     (AmpUnits)units->currentIndex()));
        last_unit_index = units->currentIndex();
    }
    emit amplitudeChanged(Amplitude(entry->GetValue(), (AmpUnits)units->currentIndex()));
}

// Time Entry Line Widget
TimeEntry::TimeEntry(const QString &label_text, Time t, TimeUnit tu, QWidget *parent) :
    QWidget(parent)
{
    time = t;
    units = tu;

    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    label = new Label(label_text, this);
    entry = new LineEntry(VALUE_ENTRY, this);

    units_label = new Label(g_time_unit_map[(int)tu].unit_str, this);
    units_label->resize(30, ENTRY_H);
    units_label->setAlignment(Qt::AlignCenter);

    connect(entry, SIGNAL(entryUpdated()), this, SLOT(entryChanged()));
}

void TimeEntry::SetTime(Time t)
{
    time = t;
    entry->SetValue(time.ChangeUnit(units));
}

void TimeEntry::resizeEvent(QResizeEvent *)
{
    int entryWidth = (width() < 200) ? 120 : qMin(width() - 80, 210);
    int lblWidth = qMax(200, width()) - entryWidth;

    // Units label does not resize just moves
    units_label->move(width() - units_label->width(), 0);

    label->move(ENTRY_OFFSET, 0);
    label->resize(lblWidth - ENTRY_OFFSET, ENTRY_H);

    entry->move(lblWidth, 0);
    entry->resize(entryWidth - units_label->width(), ENTRY_H);
}

void TimeEntry::entryChanged()
{
    time = Time(entry->GetValue(), units);
    emit timeChanged(time);
}

// Numeric entry widget
NumericEntry::NumericEntry(const QString &label_text,
                           double starting_value,
                           const QString &units_text,
                           QWidget *parent) :
    QWidget(parent)
{
    value = starting_value;

    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    label = new Label(label_text, this);
    entry = new LineEntry(VALUE_ENTRY, this);

    units_label = new Label(units_text, this);
    if(units_text.length() == 0) {
        units_label->resize(0, ENTRY_H);
    } else {
        units_label->resize(30, ENTRY_H);
    }
    units_label->setAlignment(Qt::AlignCenter);

    entry->SetValue(value);

    connect(entry, SIGNAL(entryUpdated()), this, SLOT(entryChanged()));
}

void NumericEntry::resizeEvent(QResizeEvent *)
{
    int entryWidth = (width() < 200) ? 120 : qMin(width() - 80, 210);
    int lblWidth = qMax(200, width()) - entryWidth;

    // Units label does not resize just moves
    units_label->move(width() - units_label->width(), 0);

    label->move(ENTRY_OFFSET, 0);
    label->resize(lblWidth - ENTRY_OFFSET, ENTRY_H);

    entry->move(lblWidth, 0);
    entry->resize(entryWidth - units_label->width(), ENTRY_H);
}

void NumericEntry::entryChanged()
{
    value = entry->GetValue();
    emit valueChanged(value);
}


// Combo entry
ComboEntry::ComboEntry(const QString &label_text, QWidget *parent)
    : QWidget(parent)
{
    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    label = new Label(label_text, this);
    combo_box = new ComboBox(this);

    connect(combo_box, SIGNAL(activated(int)), this, SIGNAL(comboIndexChanged(int)));
}

void ComboEntry::setComboIndex(int ix)
{
    combo_box->setCurrentIndex(ix);
}

void ComboEntry::setComboText(const QStringList &list)
{
    int ix = combo_box->currentIndex();
    combo_box->clear();
    combo_box->insertItems(0, list);
    combo_box->setCurrentIndex(ix);
}

void ComboEntry::resizeEvent(QResizeEvent *)
{
    int comboWidth = (width() < 200) ? 120 : qMin(width() - 80, 210);
    int lblWidth = qMax(200, width()) - comboWidth;

    label->move(ENTRY_OFFSET, 0);
    label->resize(lblWidth - ENTRY_OFFSET, ENTRY_H);
    combo_box->move(lblWidth, 0);
    combo_box->resize(comboWidth, ENTRY_H);
}

// Standalone color button
ColorButton::ColorButton(QWidget *parent)
    : QPushButton(parent)
{
    setObjectName("SH_ColorButton");
    color = QColor(0, 0, 0);

    connect(this, SIGNAL(clicked(bool)), this, SLOT(onClick(bool)));
}

void ColorButton::SetColor(QColor c)
{
    color = c;

    QString styleColor;
    styleColor.sprintf("background: rgb(%d,%d,%d);",
                       color.red(), color.green(), color.blue());

    setStyleSheet(styleColor);
}

void ColorButton::onClick(bool)
{
    QColor newColor = QColorDialog::getColor(color);

    if(!newColor.isValid())
        return;

    SetColor(newColor);
    emit colorChanged(color);
}

// Line Entry widget for Color Button
ColorEntry::ColorEntry(const QString &label_text, QWidget *parent)
    : QWidget(parent)
{
    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    label = new Label(label_text, this);
    color_button = new ColorButton(this);

    connect(color_button, SIGNAL(colorChanged(QColor&)),
            this, SIGNAL(colorChanged(QColor&)));
}

void ColorEntry::resizeEvent(QResizeEvent *)
{
    int btnWidth = (width() < 200) ? 120 : qMin(width() - 80, 210);
    int lblWidth = qMax(200, width()) - btnWidth;

    label->move(ENTRY_OFFSET, 0);
    label->resize(lblWidth - ENTRY_OFFSET, ENTRY_H);
    color_button->move(lblWidth, 0);
    color_button->resize(btnWidth, ENTRY_H);
}

// Line Entry for check box
CheckBoxEntry::CheckBoxEntry(const QString &label_text, QWidget *parent)
    : QWidget(parent)
{
    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    label = new Label(label_text, this);
    label->move(ENTRY_OFFSET, 0);
    label->resize(100, ENTRY_H);

    check_box = new QRadioButton(this);
    check_box->setObjectName("SHPrefRadioButton");
    //check_box->setLayoutDirection(Qt::RightToLeft);
    check_box->move(0, 0);

    connect(check_box, SIGNAL(clicked(bool)), this, SIGNAL(clicked(bool)));
}

void CheckBoxEntry::resizeEvent(QResizeEvent *)
{
    int btnWidth = (width() < 200) ? 120 : qMin(width() - 80, 210);
    int lblWidth = qMax(200, width()) - btnWidth;

    label->resize(lblWidth - ENTRY_OFFSET, ENTRY_H);
    check_box->move(lblWidth - 10, 0);
    check_box->resize(width() - lblWidth, ENTRY_H);

//    check_box->resize(width(), ENTRY_H);
}

// Dual side-by-side check boxes
DualCheckBox::DualCheckBox(const QString &left_text,
                           const QString &right_text,
                           QWidget *parent) :
    QWidget(parent),
    left(new CheckBoxEntry(left_text, this)),
    right(new CheckBoxEntry(right_text, this))
{
    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    connect(left, SIGNAL(clicked(bool)), this, SIGNAL(leftClicked(bool)));
    connect(right, SIGNAL(clicked(bool)), this, SIGNAL(rightClicked(bool)));
}

void DualCheckBox::resizeEvent(QResizeEvent *)
{
    int boxWidth = width() / 2;

    left->move(0, 0);
    left->resize(boxWidth, ENTRY_H);
    right->move(boxWidth, 0);
    right->resize(boxWidth, ENTRY_H);
}

// Dual Button Line Entry
DualButtonEntry::DualButtonEntry(const QString &left_button_title,
                                 const QString &right_button_title,
                                 QWidget *parent) :
    QWidget(parent)
{
    move(0, 0);
    resize(ENTRY_WIDTH, ENTRY_H);

    left_button = new SHPushButton(left_button_title, this);
    right_button = new SHPushButton(right_button_title, this);

    connect(left_button, SIGNAL(clicked()), this, SIGNAL(leftPressed()));
    connect(right_button, SIGNAL(clicked()), this, SIGNAL(rightPressed()));
}

void DualButtonEntry::resizeEvent(QResizeEvent *)
{
    int btnWidth = (width() - ENTRY_OFFSET)/2;

    left_button->move(ENTRY_OFFSET, 0);
    left_button->resize(btnWidth, ENTRY_H);
    right_button->move(ENTRY_OFFSET + btnWidth, 0);
    right_button->resize(btnWidth, ENTRY_H);
}
