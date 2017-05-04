#ifndef DOCKPANEL_H
#define DOCKPANEL_H

#include <QDockWidget>
#include <QScrollArea>
#include <QVector>
#include <QSettings>

#include "../lib/macros.h"
#include "dock_page.h"

class DockPanel : public QDockWidget {
    Q_OBJECT

public:
    DockPanel(const QString &title,
              QWidget *parent);
    ~DockPanel() {}

    void PrependPage(DockPage *page);
    void AppendPage(DockPage *page);
    void RemovePage(DockPage *page);
    QScrollArea* GetPanelWidget() { return scrollArea; }

    // Save and restore the collapsed state of the DockPages
    void SaveState(QSettings &);
    void RestoreState(const QSettings &);

protected:
    void resizeEvent(QResizeEvent*) {
        tabsChanged();
    }

private:
    QScrollArea *scrollArea;
    QWidget *scrollWidget;
    std::vector<DockPage*> tabs;

public slots:
    void tabsChanged();

private slots:


private:
    DISALLOW_COPY_AND_ASSIGN(DockPanel)
};

#endif // DOCKPANEL_H
