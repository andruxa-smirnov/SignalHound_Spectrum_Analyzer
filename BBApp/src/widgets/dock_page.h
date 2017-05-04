#ifndef DOCKPAGE_H
#define DOCKPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QCheckBox>

#include "../lib/macros.h"

// Modified QCheckBox to change the clickable
//   area to the full size of the widget
//
class PageTab : public QCheckBox {
    Q_OBJECT

public:
    PageTab::PageTab(const QString &label, QWidget *parent = 0) :
        QCheckBox(label, parent) {
        setObjectName("SH_PageTab");
    }
    PageTab::~PageTab() {}

    virtual bool hitButton(const QPoint &) const { return true; }
};

// Page
// Represents one collapsible tab on a given Panel
// Manages the button responsible for collapsing the tab
// Manages the state of a given tab
//  1) Dimensions
//  2) Open/Closed
//
class DockPage : public QWidget {
    Q_OBJECT

public:
    DockPage(const QString &name, QWidget *parent = 0);
    ~DockPage();

    bool TabIsOpen() const;
    void SetOpen(bool _open);
    void SetPageEnabled(bool enabled);

    // Add entry widget to the page
    void AddWidget(QWidget *new_widget);
    void RemoveWidget(QWidget *widget);

    // Get height of widget, height is determined by whether
    //  the page is open or not
    int GetTotalHeight() const;
    int TabHeight() const { return TAB_HEIGHT; }

    QString PageName() const;

protected:
    static const int TAB_ICON_SIZE = 25;
    static const int TAB_HEIGHT = 25;

    void resizeEvent(QResizeEvent *);

private:
    PageTab *tab;
    // List of widgets who belong to this page
    // Page does not own the widgets
    //QVector<QWidget*> list;
    std::vector<QWidget*> list;
    bool open;
    int pageHeight;

signals:
    void tabUpdated();

public slots:
    void updateTab() { tabToggled(open); }
    void tabToggled(bool checked);

private:
    DISALLOW_COPY_AND_ASSIGN(DockPage)
};

#endif // DOCKPAGE_H
