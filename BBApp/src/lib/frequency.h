#ifndef FREQUENCY_H
#define FREQUENCY_H

#include <QString>
#include <QVariant>

class Frequency {
public:
    enum Hertz {
        Invalid_Hz = -1, // for string matching
        Hz = 0,
        kHz = 1,
        MHz = 2,
        GHz = 3
    };

    Frequency() : frequency(-1.0) {}
    Frequency(const Frequency &other) { *this = other; }
    Frequency(double f) { frequency = f; }
    ~Frequency() {}

    // Comparisons
    bool operator<(const Frequency &other) const {
        return (frequency < other.frequency); }
    bool operator>(const Frequency &other) const {
        return (frequency > other.frequency); }
    bool operator<(const double &other) const {
        return (frequency < other); }
    bool operator>(const double &other) const {
        return (frequency > other); }

    // Conversions
    Frequency& operator=(float f) { frequency = f; return *this; }
    Frequency& operator=(double f) { frequency = f; return *this; }
    Frequency& operator=(const Frequency &other) {
        frequency = other.frequency; return *this; }
    operator double() const { return frequency; }

    // Equivalence
    bool operator==(const Frequency &other) const {
        return (frequency == other.frequency);
    }
    bool operator!=(const Frequency &other) const {
        return !(*this == other);
    }
    Frequency& operator+=(const Frequency &other) {
        frequency += other.Val();
        return *this;
    }
    Frequency& operator+=(double other) {
        frequency += other;
        return *this;
    }
    Frequency& operator-=(const Frequency &other) {
        frequency -= other.Val();
        return *this;
    }
    Frequency& operator-=(double other) {
        frequency -= other;
        return *this;
    }
    Frequency operator+(const Frequency &other) const {
        return Frequency(frequency + other.Val());
    }
    Frequency operator+(double other) const {
        return Frequency(frequency + other);
    }
    Frequency operator-(const Frequency &other) {
        return Frequency(frequency - other.Val());
    }
    Frequency operator-(double other) {
        return Frequency(frequency - other);
    }

    static bool IsValidFreqString(QString str, Frequency &f);
    void Set(double f, Hertz h);

    double Val() const { return frequency; }
    void Clamp(Frequency min, Frequency max) {
        if(Val() < min.Val()) frequency = min.Val();
        if(Val() > max.Val()) frequency = max.Val();
    }

    // Get Unit (String) of current value
    Hertz GetUnits() const;
    QString GetUnitString() const;

    // Simple string, new copy
    QString GetFreqString(void) const {
        QString s;
        CreateSimpleString(s);
        return s;
    }

    // Simple string, no copy constructors
    QString& GetFreqString(QString &s) const {
        CreateSimpleString(s);
        return s;
    }

    // Complex string, new copy
    QString GetFreqString(int precision, bool units) const {
        QString s;
        CreateComplexString(s, precision, units);
        return s;
    }

    // Complex string, no copy constructors
    QString& GetFreqString(QString& s, int precision, bool units) const {
        CreateComplexString(s, precision, units);
        return s;
    }

private:
    // The value, and only variable
    double frequency;

    // Seven digits after the decimal, no unit string, weak negatives
    void CreateSimpleString(QString &s) const {
        bool neg = false;
        double freq = frequency;

        if(frequency < 0.0) {
            neg = true;
            freq *= -1.0;
        }

        if(freq < 1.0e3)
            s.sprintf("%s%.6f Hz", neg?"-":"", freq);
        else if(freq < 1.0e6)
            s.sprintf("%s%.6f kHz", neg?"-":"", freq * 0.001);
        else if(freq < 1.0e9)
            s.sprintf("%s%.6f MHz", neg?"-":"", freq * 1.0e-6);
        else
            s.sprintf("%s%.6f GHz", neg?"-":"", freq * 1.0e-9);
    }

    // Variable digits after decimal and optional unit string, and negatives
    void CreateComplexString(QString &s, int precision, bool units) const {

        bool neg = false;
        double freq = frequency;

        if(frequency < 0.0) {
            neg = true;
            freq *= -1.0;
        }

        if(freq < 1.0e3)
            s.sprintf("%s%.*lf%s", neg?"-":"", precision, freq, units?" Hz":"");
        else if(freq < 1.0e6)
            s.sprintf("%s%.*lf%s", neg?"-":"", precision, freq * 1.0e-3, units?" kHz":"");
        else if(freq < 1.0e9)
            s.sprintf("%s%.*lf%s", neg?"-":"", precision, freq * 1.0e-6, units?" MHz":"");
        else
            s.sprintf("%s%.*lf%s", neg?"-":"", precision, freq * 1.0e-9, units?" GHz":"");
    }
};

#endif // FREQUENCY_H
