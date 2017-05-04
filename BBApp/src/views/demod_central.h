#ifndef DEMOD_CENTRAL_H
#define DEMOD_CENTRAL_H

#include <QToolBar>
#include <QBoxLayout>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPaintEvent>
#include <QMessageBox>

#include <mutex>

#include "lib/bb_lib.h"
#include "model/session.h"
#include "central_stack.h"
#include "gl_sub_view.h"

class MdiArea : public QMdiArea {
    Q_OBJECT

public:
    MdiArea(QWidget *parent = 0) : QMdiArea(parent) {}
    ~MdiArea() {}

    void stacked_retile() {
        auto list = subWindowList();

        int viewHeight = height() / list.size();
        int viewCount = 0;

        for(auto view : list) {
            view->move(0, viewCount * viewHeight);
            view->resize(width(), viewHeight);
            viewCount++;
        }
    }

    // Stacked w/ side-by-side
    void retile() {
        QList<QMdiSubWindow*> list = subWindowList();
        if(list.length() != 3) return;
        list.at(0)->move(0, 0);
        list.at(0)->resize(width(), height() / 2);
        list.at(1)->move(0, height() / 2);
        list.at(1)->resize(width() / 2, height() / 2 + (height() % 2));
        list.at(2)->move(width() / 2, height() / 2);
        list.at(2)->resize(width() / 2 + (width() % 2), height() / 2 + (height() % 2));
    }

    std::mutex viewLock;

public slots:
    void updateViews() {
        {
            std::lock_guard<std::mutex> guard(viewLock);
            QList<QMdiSubWindow*> list = subWindowList();
            for(QMdiSubWindow *view : list) {
                view->widget()->update();
            }
        }
    }

private:
    DISALLOW_COPY_AND_ASSIGN(MdiArea)
};

//class CircularBuffer {
//public:
//    CircularBuffer();
//    ~CircularBuffer();

//    void Resize(int captureSize);
//    void Store(std::vector<complex_f> &src);
//    void GetCapture(IQSweep &sweep);
//    void GetData(complex_f *dst, int len);

//private:
//    static const int TOTAL_PACKETS = 40;
//    int packetLen;
//    std::mutex bufferLock;
//    semaphore packetRecieved;
//    std::atomic<int> available;
//    std::vector<complex_f> buffer;
//    int storeIx, retreiveIx;
//};

class DemodCentral : public CentralWidget {
    Q_OBJECT

public:
    DemodCentral(Session *sPtr,
                 QToolBar *mainToolBar,
                 QWidget *parent = 0,
                 Qt::WindowFlags f = 0);
    ~DemodCentral();

    void StartStreaming();
    void StopStreaming();
    void ResetView();
    void GetViewImage(QImage &image);
    Frequency GetCurrentCenterFreq() const;

protected:
    void resizeEvent(QResizeEvent *);

private:
    void Reconfigure(DemodSettings *ds, IQCapture *iqc, IQSweep &iqSweep);
    bool GetCapture(const DemodSettings *ds, IQCapture &iqc,
                    IQSweep &iq, Device *device);
    //void CollectThread(Device *device, int captureLen);
    void StreamThread();
    void UpdateView();
    void RecordIQCapture(const IQSweep &sweep, IQCapture &capture, Device *device);

    Session *sessionPtr; // Copy, does not own
    //CircularBuffer circularBuffer;

    QToolBar *recordToolBar;
    MdiArea *demodArea;

    DemodSettings lastConfig;
    std::atomic<int> captureCount;
    std::thread threadHandle;
    bool streaming;
    bool reconfigure;
    std::vector<complex_f> buffer;

    Label *currentRecordDirLabel;
    LineEntry *recordLenEntry;
    QString currentRecordDir;
    double recordLength; // Record length in time
    bool recordBinary;
    std::atomic<bool> recordNext;
    QMessageBox recordDialog;

public slots:
    void changeMode(int newState);
    void updateSettings(const DemodSettings *ds);

private slots:
    void singlePressed();
    void autoPressed();
    void saveAsType(int type) { recordBinary = (type == 0); }
    void recordPressed();
    void updateRecordingDialog(bool show);

    void changeRecordDirectory();
    void recordLengthChanged();

signals:
    void updateViews();
    void showRecordingDialog(bool);

private:
    DISALLOW_COPY_AND_ASSIGN(DemodCentral)
};

#endif // DEMOD_CENTRAL_H
