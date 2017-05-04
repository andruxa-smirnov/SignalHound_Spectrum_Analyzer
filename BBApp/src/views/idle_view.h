#ifndef IDLE_VIEW_H
#define IDLE_VIEW_H

#include <QWidget>
#include "../lib/bb_lib.h"

class IdleView : public QWidget {
    Q_OBJECT

public:
    IdleView();
    ~IdleView();

    void ConnectDevice();

protected:

private:

public slots:

private slots:

signals:

private:
    DISALLOW_COPY_AND_ASSIGN(IdleView)
};

#endif // IDLE_VIEW_H
