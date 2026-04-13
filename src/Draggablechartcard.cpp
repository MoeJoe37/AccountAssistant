#include "draggablechartcard.h"
#include "translations.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QPixmap>
#include <QPainter>
#include <QLabel>
#include <QStyle>
#include <QMenu>
#include <QContextMenuEvent>

static const char* kCardSSDark = R"(
QFrame#card {
    background:#1a1f38;
    border:1px solid #252b52;
    border-radius:12px;
}
QFrame#card[highlighted="true"] {
    border:2px solid #4f86f7;
}
QWidget#handle {
    background:qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #1e264a, stop:1 #1a1f38);
    border-radius:10px 10px 0 0;
    border-bottom:1px solid #252b52;
}
QLabel#cardTitle {
    color:#c8d0ed;
    font-size:13px; font-weight:700;
    background:transparent;
}
QLabel#dragHint {
    color:#3a4470;
    font-size:18px;
    background:transparent;
    padding:0 6px 0 0;
}
)";

static const char* kCardSSLight = R"(
QFrame#card {
    background:#ffffff;
    border:1px solid #dde2f0;
    border-radius:12px;
}
QFrame#card[highlighted="true"] {
    border:2px solid #4f86f7;
}
QWidget#handle {
    background:qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #eef0fa, stop:1 #ffffff);
    border-radius:10px 10px 0 0;
    border-bottom:1px solid #dde2f0;
}
QLabel#cardTitle {
    color:#1e2340;
    font-size:13px; font-weight:700;
    background:transparent;
}
QLabel#dragHint {
    color:#c8d0ed;
    font-size:18px;
    background:transparent;
    padding:0 6px 0 0;
}
)";

DraggableChartCard::DraggableChartCard(const QString& title,
                                       QChartView*    view,
                                       QWidget*       parent)
    : QFrame(parent), m_view(view), m_title(title)
{
    setObjectName("card");
    setStyleSheet(g_lightMode ? kCardSSLight : kCardSSDark);
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::DefaultContextMenu);

    auto* vl = new QVBoxLayout(this);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(0);

    m_handle = new QWidget;
    m_handle->setObjectName("handle");
    m_handle->setFixedHeight(38);
    m_handle->setCursor(Qt::OpenHandCursor);
    m_handle->setMouseTracking(true);

    auto* hl = new QHBoxLayout(m_handle);
    hl->setContentsMargins(14, 0, 14, 0);
    hl->setSpacing(8);

    auto* hint = new QLabel("⠿");
    hint->setObjectName("dragHint");
    hl->addWidget(hint);

    m_titleLabel = new QLabel(title);
    m_titleLabel->setObjectName("cardTitle");
    hl->addWidget(m_titleLabel, 1);

    vl->addWidget(m_handle);

    view->setParent(this);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    vl->addWidget(view);
}

void DraggableChartCard::setHighlight(bool on)
{
    setProperty("highlighted", on ? "true" : "false");
    style()->unpolish(this);
    style()->polish(this);
}

void DraggableChartCard::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_onHandle = m_handle->geometry().contains(e->pos());
        if (m_onHandle) {
            m_dragStart = e->pos();
            m_handle->setCursor(Qt::ClosedHandCursor);
        }
    }
    QFrame::mousePressEvent(e);
}

void DraggableChartCard::mouseMoveEvent(QMouseEvent* e)
{
    if (!(e->buttons() & Qt::LeftButton) || !m_onHandle) {
        QFrame::mouseMoveEvent(e);
        return;
    }
    if ((e->pos() - m_dragStart).manhattanLength() < QApplication::startDragDistance()) {
        QFrame::mouseMoveEvent(e);
        return;
    }

    QPixmap px = grab();
    QPixmap ghost = px.scaled(px.width() * 3 / 5,
                              px.height() * 3 / 5,
                              Qt::KeepAspectRatio,
                              Qt::SmoothTransformation);
    QPixmap faded(ghost.size());
    faded.fill(Qt::transparent);
    QPainter p(&faded);
    p.setOpacity(0.75);
    p.drawPixmap(0, 0, ghost);

    auto* drag = new QDrag(this);
    auto* mime = new QMimeData;
    mime->setData("application/x-account-flow-item", QByteArray::number(m_flowIndex));
    drag->setMimeData(mime);
    drag->setPixmap(faded);
    drag->setHotSpot({ faded.width() / 2, 20 });
    drag->exec(Qt::MoveAction);

    m_handle->setCursor(Qt::OpenHandCursor);
    m_onHandle = false;
}

void DraggableChartCard::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasFormat("application/x-account-flow-item")) {
        int fromIdx = e->mimeData()->data("application/x-account-flow-item").toInt();
        if (fromIdx != m_flowIndex) {
            e->acceptProposedAction();
            setHighlight(true);
        }
    }
}

void DraggableChartCard::dragLeaveEvent(QDragLeaveEvent*)
{
    setHighlight(false);
}

void DraggableChartCard::dropEvent(QDropEvent* e)
{
    setHighlight(false);
    if (e->mimeData()->hasFormat("application/x-account-flow-item")) {
        int fromIdx = e->mimeData()->data("application/x-account-flow-item").toInt();
        if (fromIdx != m_flowIndex) {
            e->acceptProposedAction();
            emit swapRequested(fromIdx, m_flowIndex);
        }
    }
}

void DraggableChartCard::contextMenuEvent(QContextMenuEvent* e)
{
    QMenu menu(this);
    QAction* insertSep = menu.addAction(T("Add page separator below", "إضافة فاصل صفحة أسفلها"));
    QAction* hideAct = menu.addAction(T("Hide chart", "إخفاء الرسم"));
    QAction* chosen = menu.exec(e->globalPos());
    if (chosen == insertSep) {
        emit insertSeparatorRequested(m_flowIndex);
    } else if (chosen == hideAct) {
        emit hideRequested(m_index);
    }
}
