#include "OriMruMenu.h"

#include "../tools/OriMruList.h"

#include <QAction>
#include <QBoxLayout>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>

namespace Ori {
namespace Widgets {

//------------------------------------------------------------------------------
//                                 MruMenu
//------------------------------------------------------------------------------

MruMenu::MruMenu(const QString& title, MruList *mru, QWidget *parent) : QMenu(title, parent), _mru(mru)
{
    if (_mru)
        connect(_mru, &Ori::MruList::changed, this, &MruMenu::populate);

    populate();
}

MruMenu::MruMenu(MruList *mru, QWidget *parent) : MruMenu(tr("Recently used"), mru, parent)
{
}

void MruMenu::populate()
{
    clear();

    if (!_mru) return;

    if (!_mru->actions().isEmpty())
    {
        for (auto a : _mru->actions()) addAction(a);
        addSeparator();
        addAction(_mru->actionClearInvalids());
        addAction(_mru->actionClearAll());
        setEnabled(true);
    }
    else
        setEnabled(false);
}

//------------------------------------------------------------------------------
//                               MruMenuPart
//------------------------------------------------------------------------------

MruMenuPart::MruMenuPart(MruList *mru, QMenu *menu, QAction *placeholder, QWidget *parent)
    : QObject(parent), _mru(mru), _menu(menu), _placeholder(placeholder)
{
    if (_mru)
        connect(_mru, &Ori::MruList::changed, this, &MruMenuPart::populate);

    _separator = new QAction(this);
    _separator->setSeparator(true);

    populate();
}

void MruMenuPart::populate()
{
    if (!_mru || !_menu || !_placeholder) return;

    for (auto a : _mru->actions()) _menu->removeAction(a);
    _menu->removeAction(_mru->actionClearAll());
    _menu->removeAction(_mru->actionClearInvalids());

    if (!_mru->actions().isEmpty())
    {
        _menu->insertAction(_placeholder, _separator);
        _menu->insertAction(_separator, _mru->actionClearAll());
        _menu->insertAction(_mru->actionClearAll(), _mru->actionClearInvalids());
        QAction* place = _mru->actionClearInvalids();
        for (int i = _mru->actions().size()-1; i >= 0; i--)
        {
            auto action = _mru->actions().at(i);
            _menu->insertAction(place, action);
            place = action;
        }
    }
}

//------------------------------------------------------------------------------
//                              MruListWidget
//------------------------------------------------------------------------------

MruListWidget::MruListWidget(MruList *mru, QWidget *parent) : QWidget(parent), _mru(mru)
{
    if (_mru)
        connect(_mru, &Ori::MruList::changed, this, &MruListWidget::populate);

    _header = new QLabel;
    setHeader(tr("Recently used"));

    _layout = new QVBoxLayout;
    _layout->setSpacing(6);
    _layout->addWidget(_header);

    auto layoutCenter = new QHBoxLayout;
    layoutCenter->setContentsMargins(0, 0, 0, 0);
    layoutCenter->setSpacing(0);
    layoutCenter->addStretch();
    layoutCenter->addLayout(_layout);
    layoutCenter->addStretch();

    auto layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(0, 0, 0, 0);
    layoutMain->setSpacing(0);
    layoutMain->addStretch();
    layoutMain->addLayout(layoutCenter);
    layoutMain->addStretch();
    setLayout(layoutMain);

    populate();
}

void MruListWidget::populate()
{
    qDeleteAll(_links.keyBegin(), _links.keyEnd());
    _links.clear();

    if (!_mru) return;

    foreach (QAction *action, _mru->actions())
    {
        // TODO: There is the call to virtual method from constructor
        // check where the derived class MruFileListWidget is used and fix the flow
        auto link = new QLabel(makeLinkText(action));
        connect(link, &QLabel::linkActivated, this, &MruListWidget::clicked);
        _layout->addWidget(link);
        _links.insert(link, action);
    }
}

QString MruListWidget::makeLinkText(QAction* action) const
{
    return QString("<a href=%1>%1</a>").arg(action->text());
}

void MruListWidget::clicked()
{
    if (_links.contains(sender()))
        _links[sender()]->trigger();
}

void MruListWidget::setHeader(const QString& s)
{
    _header->setText(QString("<b>%1</b>").arg(s));
}

//------------------------------------------------------------------------------
//                              MruFileListWidget
//------------------------------------------------------------------------------

QString MruFileListWidget::makeLinkText(QAction* action) const
{
    QFileInfo file(action->text());
    auto text = file.exists()
        ? QString("<a href=%1>%1</a>").arg(file.completeBaseName())
        : file.completeBaseName();
    QString pathColor = palette().color(QPalette::Shadow).name();
    return QString("%1<br><font color='%3'>%2</font>").arg(text, file.filePath(), pathColor);
}

} // namespace Widgets
} // namespace Ori
