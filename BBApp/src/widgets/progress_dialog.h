#ifndef PROGRESS_DIALOG_H
#define PROGRESS_DIALOG_H

#include <QProgressDialog>
#include <QTimer>
#include <QLabel>

#include <thread>

class SHProgressDialog : public QProgressDialog {
    Q_OBJECT

public:
    SHProgressDialog(QString text, QWidget *parent = 0) :
        QProgressDialog(text, QString(), 0, 0, parent/*, Qt::WindowStaysOnTopHint*/)
    {
        setModal(true);
    }
};

// N/A Stupid multi-threaded progress dialog idea
//class ProgressDialog : public QProgressDialog {
//    Q_OBJECT

//public:
//    ProgressDialog(QWidget *parent = 0) :
//        QProgressDialog("", QString(), 0, 0, parent)
//    {
//        setWindowTitle("Initialization");
//        setModal(true);
//    }

//    ~ProgressDialog() {}

//    void makeVisible(const QString &label) {
//        setLabelText(label);
//        QMetaObject::invokeMethod(this, "show");
//    }

//    void makeDisappear() {
//        QMetaObject::invokeMethod(this, "hide");
//    }

//private:

//};

#endif // PROGRESS_DIALOG_H
