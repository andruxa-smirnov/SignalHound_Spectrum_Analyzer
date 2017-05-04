#include "dock_page.h"
#include "dock_panel.h"

DockPage::DockPage(const QString &name, QWidget *parent)
    : QWidget(parent, 0)
{
    setObjectName("SH_Page");
    open = true;

    // 5 pixels between tab and first entry widget
    pageHeight = TAB_HEIGHT + 5;

    tab = new PageTab(name, this);
    tab->setIconSize(QSize(20, 20));
    tab->move(0, 0);
    tab->resize(MAX_DOCK_WIDTH, TAB_HEIGHT);

    connect(tab, SIGNAL(toggled(bool)), this, SLOT(tabToggled(bool)));

    tab->setChecked(true);
}

DockPage::~DockPage()
{

}

bool DockPage::TabIsOpen() const
{
    return tab->isChecked();
}

// By calling setChecked() the button->toggled(bool)
//  signal is fired, calling tabToggled() below
void DockPage::SetOpen(bool new_state)
{
    tab->setChecked(new_state);
}

void DockPage::SetPageEnabled(bool enabled)
{
    for(QWidget *w : list) {
        w->setEnabled(enabled);
    }
}

void DockPage::AddWidget(QWidget *new_widget)
{
    new_widget->setParent(this);
    list.push_back(new_widget);
    new_widget->move(0, pageHeight);
    pageHeight += new_widget->height();
}

void DockPage::RemoveWidget(QWidget *widget)
{
    std::vector<QWidget*> li;
    auto iter = std::find(list.begin(), list.end(), widget);
    if(iter != list.end()) {
        list.erase(iter);
        widget->setParent(0);
        pageHeight -= widget->height();
    }
}

int DockPage::GetTotalHeight() const
{
    int h = 0;

    if(TabIsOpen()) {
        h += pageHeight;
    }

    return TAB_ICON_SIZE + h;
}

QString DockPage::PageName() const
{
    return tab->text();
}

void DockPage::resizeEvent(QResizeEvent *)
{
    tab->resize(width(), TAB_HEIGHT);

    for(QWidget *widget : list) {
        widget->resize(width() - 10, ENTRY_H);
    }
}

void DockPage::tabToggled(bool checked)
{
    open = checked;

//    if(open) {
//        tab->setIcon(QIcon(":icons/collapse-down.png"));
//    } else {
//        tab->setIcon(QIcon(":icons/collapse-right.png"));
//    }

    emit tabUpdated();
}
