#include "dock_panel.h"

DockPanel::DockPanel(const QString &title,
                     QWidget *parent) :
    QDockWidget(title, parent, Qt::Widget)
{
    // Minimal features, no floating
    setFeatures(QDockWidget::DockWidgetMovable);

    setMaximumWidth(MAX_DOCK_WIDTH);
    setMinimumWidth(MIN_DOCK_WIDTH);

    QString objectName = title + "Panel";

    //setTi

    scrollArea = new QScrollArea();
    scrollArea->move(0, 0);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setWidget(scrollArea);

    scrollWidget = new QWidget(scrollArea);
    scrollWidget->setObjectName("SH_Panel");
    scrollArea->setWidget(scrollWidget);

    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setAllowedAreas(Qt::RightDockWidgetArea |
                    Qt::LeftDockWidgetArea);
}

void DockPanel::PrependPage(DockPage *page)
{
    // Do not add page if it already is in the list
    // Loading presets might cause this function to get
    //  called again even if the page already is in the list.
    auto iter = std::find(tabs.begin(), tabs.end(), page);
    if(iter != tabs.end()) {
        return;
    }

    connect(page, SIGNAL(tabUpdated()),
            this, SLOT(tabsChanged()));

    tabs.insert(tabs.begin(), page);
    page->setParent(scrollArea->widget());

    tabsChanged();
}

void DockPanel::AppendPage(DockPage *page)
{
    // Do not add page if it already is in the list
    // Loading presets might cause this function to get
    //  called again even if the page already is in the list.
    auto iter = std::find(tabs.begin(), tabs.end(), page);
    if(iter != tabs.end()) {
        return;
    }

    connect(page, SIGNAL(tabUpdated()),
            this, SLOT(tabsChanged()));

    tabs.push_back(page);
    page->setParent(scrollArea->widget());

    tabsChanged();
}

void DockPanel::RemovePage(DockPage *page)
{
    auto iter = std::find(tabs.begin(), tabs.end(), page);
    if(iter == tabs.end()) {
        return;
    }

    disconnect(page, SIGNAL(tabUpdated()),
               this, SLOT(tabsChanged()));
    tabs.erase(iter);
    page->setParent(0);

    tabsChanged();
}

void DockPanel::SaveState(QSettings &s)
{
    for(DockPage *page : tabs) {
        QString key = "PanelTabs/" +
                windowTitle() + "/" +
                page->PageName();
        s.setValue(key, page->TabIsOpen());
    }
}

void DockPanel::RestoreState(const QSettings &s)
{
    for(DockPage *page : tabs) {
        QString key = "PanelTabs/" +
                windowTitle() + "/" +
                page->PageName();
        QVariant value = s.value(key, true);
        page->SetOpen(value.toBool());
    }
}

void DockPanel::tabsChanged()
{
    int height = 10;
    int pageHeight = 0;

    int viewportWidth = scrollArea->viewport()->width();

    for(int i = 0; i < tabs.size(); i++) {
        if(tabs[i]->TabIsOpen()) {
            pageHeight = tabs[i]->GetTotalHeight();
        } else {
            pageHeight = tabs[i]->TabHeight();
        }
        tabs[i]->resize(viewportWidth, pageHeight);
        tabs[i]->move(0, height);
        tabs[i]->show();
        height += pageHeight + 1;
    }

    scrollWidget->resize(viewportWidth, height);
}
