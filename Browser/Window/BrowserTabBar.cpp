#include "BrowserTabBar.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "WebView.h"

#include <QApplication>
#include <QDrag>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShortcut>
#include <QStylePainter>
#include <QToolButton>

BrowserTabBar::BrowserTabBar(QWidget *parent) :
    QTabBar(parent),
    m_buttonNewTab(nullptr),
    m_dragStartPos(),
    m_dragUrl(),
    m_dragPixmap(),
    m_externalDropInfo()
{
    setAcceptDrops(true);
    setDocumentMode(true);
    setExpanding(false);
    setTabsClosable(true);
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
    setUsesScrollButtons(false);
    setElideMode(Qt::ElideRight);

    // Add "New Tab" button
    m_buttonNewTab = new QToolButton(this);
    m_buttonNewTab->setIcon(QIcon::fromTheme("folder-new"));
    m_buttonNewTab->setStyleSheet("QToolButton:hover { border: 1px solid #666666; border-radius: 2px; } ");
    m_buttonNewTab->setToolTip(tr("New Tab"));
    m_buttonNewTab->setFixedSize(28, height() - 2);

    //setStyleSheet("QTabBar::tab:disabled { background-color: rgba(0, 0, 0, 0); }");
    connect(m_buttonNewTab, &QToolButton::clicked, this, &BrowserTabBar::newTabRequest);

    // Add shortcut (Ctrl+Tab) to switch between tabs
    QShortcut *tabShortcut = new QShortcut(QKeySequence(QKeySequence::NextChild), this);
    connect(tabShortcut, &QShortcut::activated, this, &BrowserTabBar::onNextTabShortcut);

    // Custom context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &BrowserTabBar::customContextMenuRequested, this, &BrowserTabBar::onContextMenuRequest);
}

void BrowserTabBar::onNextTabShortcut()
{
    int nextIdx = currentIndex() + 1;

    if (nextIdx >= count())
        setCurrentIndex(0);
    else
        setCurrentIndex(nextIdx);
}

void BrowserTabBar::onContextMenuRequest(const QPoint &pos)
{
    QMenu menu(this);

    // Add "New Tab" menu item, shown on every context menu request
    menu.addAction(tr("New Tab"), this, &BrowserTabBar::newTabRequest);

    // Check if the user right-clicked on a tab, or just some position on the tab bar
    int tabIndex = tabAt(pos);
    if (tabIndex >= 0)
    {
        menu.addSeparator();
        menu.addAction(tr("Close Tab"), [=](){
            removeTab(tabIndex);
        });
        menu.addSeparator();
        menu.addAction(tr("Reload"), [=]() {
            emit reloadTabRequest(tabIndex);
        });
    }

    menu.exec(mapToGlobal(pos));
}

void BrowserTabBar::mousePressEvent(QMouseEvent *event)
{
    int tabIdx = tabAt(event->pos());
    if (event->button() == Qt::LeftButton
            && tabIdx >= 0)
    {
        if (BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(parentWidget()))
        {
            m_dragStartPos = event->pos();
            m_dragUrl = tabWidget->getWebView(tabIdx)->url();

            QRect dragTabRect = tabRect(tabIdx);
            m_dragPixmap = QPixmap(dragTabRect.size());
            render(&m_dragPixmap, QPoint(), QRegion(dragTabRect));
        }
    }

    QTabBar::mousePressEvent(event);
}

void BrowserTabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
    {
        QTabBar::mouseMoveEvent(event);
        return;
    }
    if (std::abs(event->pos().y() - m_dragStartPos.y()) < height())
    {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    // Set mime data and initiate drag event
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;

    int tabIdx = tabAt(m_dragStartPos);
    mimeData->setData("application/x-browser-tab", m_dragUrl.toEncoded());
    mimeData->setProperty("tab-origin-window-id", window()->winId());
    drag->setMimeData(mimeData);
    drag->setPixmap(m_dragPixmap);

    Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
    if (dropAction == Qt::MoveAction)
    {
        // If the tab was moved, and it was the only tab, close the window
        if (count() == 1)
        {
            window()->close();
        }
        else
        {
            BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(parentWidget());
            WebView *view = tabWidget->getWebView(tabIdx);
            tabWidget->removeTab(tabIdx);
            view->deleteLater();
        }
    }

    QTabBar::mouseMoveEvent(event);
}

void BrowserTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls() || mimeData->hasFormat("application/x-browser-tab"))
    {
        event->acceptProposedAction();
        return;
    }

    QTabBar::dragEnterEvent(event);
}

void BrowserTabBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_externalDropInfo.NearestTabIndex = -1;

    QTabBar::dragLeaveEvent(event);

    update();
}

void BrowserTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        const QPoint eventPos = event->pos();

        int nearestTabIndex = tabAt(eventPos);
        if (nearestTabIndex < 0)
            nearestTabIndex = count() - 1;

        const QRect r = tabRect(nearestTabIndex);
        const QPoint tabCenterPos = r.center();
        const int quarterTabWidth = 0.25f * r.width();

        DropIndicatorLocation location = DropIndicatorLocation::Center;
        if (eventPos.x() > tabCenterPos.x() + quarterTabWidth)
            location = DropIndicatorLocation::Right;
        else if (eventPos.x() < tabCenterPos.x() - quarterTabWidth)
            location = DropIndicatorLocation::Left;

        m_externalDropInfo.NearestTabIndex = nearestTabIndex;
        m_externalDropInfo.Location = location;

        update();
    }

    QTabBar::dragMoveEvent(event);
}

void BrowserTabBar::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(parentWidget());
        if (!tabWidget)
            return;

        int newTabIndex = m_externalDropInfo.NearestTabIndex;

        QList<QUrl> urls = mimeData->urls();
        for (const auto &url : urls)
        {
            if (m_externalDropInfo.Location == DropIndicatorLocation::Center)
            {
                WebView *view = tabWidget->getWebView(newTabIndex);
                view->load(url);

                m_externalDropInfo.Location = DropIndicatorLocation::Right;
            }
            else
            {
                int tabOffset = (m_externalDropInfo.Location == DropIndicatorLocation::Right) ? 1 : 0;
                WebView *view = tabWidget->newTab(false, true, newTabIndex + tabOffset);
                view->load(url);

                newTabIndex += tabOffset;
            }
        }

        event->acceptProposedAction();
        m_externalDropInfo.NearestTabIndex = -1;
        return;
    }
    else if (mimeData->hasFormat("application/x-browser-tab"))
    {
        qobject_cast<MainWindow*>(window())->dropEvent(event);
        return;
    }

    QTabBar::dropEvent(event);
}

void BrowserTabBar::paintEvent(QPaintEvent *event)
{
    QTabBar::paintEvent(event);

    if (m_externalDropInfo.NearestTabIndex >= 0)
    {
        QPainter p(this);

        const QRect r = tabRect(m_externalDropInfo.NearestTabIndex);
        QPoint indicatorPos(0, r.center().y());

        switch (m_externalDropInfo.Location)
        {
            case DropIndicatorLocation::Center:
                indicatorPos.setX(r.center().x());
                break;
            case DropIndicatorLocation::Left:
            {
                indicatorPos.setX(r.left());
                if (r.left() - 8 <= 0)
                    indicatorPos.setX(r.left() + 8);
                break;
            }
            case DropIndicatorLocation::Right:
            {
                indicatorPos.setX(r.right());
                if (r.right() + 8 >= m_buttonNewTab->pos().x())
                    indicatorPos.setX(r.right() - 5);
                break;
            }
        }

        p.setPen(QColor(Qt::black));
        p.setBrush(QBrush(QColor(Qt::white)));

        int cx = indicatorPos.x(), cy = indicatorPos.y() - 4;
        p.drawRect(cx - 4, cy - 8, 8, 4);

        QPainterPath path;
        path.moveTo(cx, indicatorPos.y());
        path.lineTo(std::max(0, cx - 8), cy - 4);
        path.lineTo(cx + 8, cy - 4);
        path.lineTo(cx, cy + 4);
        p.drawPath(path);

        p.fillRect(cx - 3, cy - 5, 7, 2, QBrush(QColor(Qt::white)));
    }
}

QSize BrowserTabBar::sizeHint() const
{
    QSize hint = QTabBar::sizeHint();
    hint.setWidth(hint.width() - 2 - m_buttonNewTab->width());
    return hint;
}

void BrowserTabBar::tabLayoutChange()
{
    QTabBar::tabLayoutChange();
    moveNewTabButton();
}

QSize BrowserTabBar::tabSizeHint(int index) const
{
    // Get the QTabBar size hint and keep width within an upper bound
    QSize hint = QTabBar::tabSizeHint(index);
    //QFontMetrics fMetric = fontMetrics();

    const int numTabs = count();
    /*const int activeTabWidth = fMetric.width("R") * 20;

    int tabWidth = 0;
    if (index == currentIndex())
        tabWidth = activeTabWidth;
    else
        tabWidth = (geometry().width() - activeTabWidth - m_buttonNewTab->width()) / (std::max(1, numTabs - 1));*/
    int tabWidth = float(geometry().width() - m_buttonNewTab->width() - 1) / numTabs;

    return hint.boundedTo(QSize(tabWidth, hint.height()));
}

void BrowserTabBar::resizeEvent(QResizeEvent *event)
{
    QTabBar::resizeEvent(event);
    moveNewTabButton();
}

void BrowserTabBar::moveNewTabButton()
{
    int numTabs = count();
    if (numTabs == 0)
        return;

    int tabWidth = 2;
    for (int i = 0; i < numTabs; ++i)
        tabWidth += tabRect(i).width();

    QRect barRect = rect();
    if (tabWidth > width())
        m_buttonNewTab->move(barRect.right() - m_buttonNewTab->width(), barRect.y()); //m_buttonNewTab->hide();
    else
    {
        m_buttonNewTab->move(barRect.left() + tabWidth, barRect.y());
        m_buttonNewTab->show();
    }
}