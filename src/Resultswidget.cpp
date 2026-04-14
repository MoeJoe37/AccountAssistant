#include "resultswidget.h"
#include "translations.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QChart>
#include <QPieSeries>
#include <QPieSlice>
#include <QLegendMarker>
#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QCategoryAxis>
#include <QLineSeries>
#include <QFont>
#include <QColor>
#include <QPainter>
#include <QMouseEvent>
#include <QDragMoveEvent>
#include <QLabel>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QContextMenuEvent>
#include <QGridLayout>
#include <QScrollBar>
#include <QFrame>
#include <QToolButton>
#include <QAction>
#include <QActionGroup>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QSizePolicy>
#include <QSet>
#include <QTableWidgetItem>
#include <algorithm>

using namespace Qt::StringLiterals;

// ─── Stay-open menu (doesn't close when clicking checkable items) ─────────────
class StayOpenMenu : public QMenu {
public:
    using QMenu::QMenu;
protected:
    void mouseReleaseEvent(QMouseEvent* e) override {
        QAction* a = activeAction();
        if (a && a->isEnabled()) {
            if (a->isCheckable()) {
                a->trigger();
            } else {
                a->trigger();
                QMenu::mouseReleaseEvent(e); // non-checkable items close menu normally
                return;
            }
            return; // stay open for checkable items
        }
        QMenu::mouseReleaseEvent(e);
    }
};

static const QList<QColor> kPal = {
    "#4f86f7", "#f0a500", "#e05c6a", "#3ecf8e",
    "#9b6cf9", "#f06c6c", "#62c4e3", "#b0e96a",
    "#ff9f43", "#fd79a8", "#00cec9", "#fdcb6e"
};

static const char* kResultsSSDark = R"(
QWidget#resultsRoot { background:#0d1020; }
QWidget#summaryBar {
    background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #131729, stop:1 #0d1020);
    border-bottom:1px solid #1e2445;
}
QWidget#sumCard {
    background:#1a1f38;
    border-radius:10px;
    border:1px solid #252b52;
    padding:14px 22px;
}
QLabel#sumTitle {
    color:#5a6490; font-weight:700;
    letter-spacing:1px; background:transparent;
}
QLabel#sumValue { font-weight:900; background:transparent; }
QToolButton#hiddenChartsBtn, QToolButton#monthSelectBtn, QToolButton#pageModeBtn {
    background:#1a1f38; color:#c8d0ed; border:1px solid #252b52;
    border-radius:8px; padding:8px 14px; font-weight:700;
}
QToolButton#hiddenChartsBtn:hover, QToolButton#monthSelectBtn:hover, QToolButton#pageModeBtn:hover { background:#1e2445; }
QWidget#gridContainer { background:#0d1020; }
QWidget#reportSection, QWidget#pageBreakSection, QWidget#chartsSection { background:transparent; }
QFrame#pageBreakLine { background:#252b52; min-height:1px; max-height:1px; border:none; }
QLabel#pageBreakLabel { color:#5a6490; font-weight:800; letter-spacing:1px; background:transparent; }
QLabel#sectionTitle { color:#c8d0ed; font-weight:800; }
QLabel#sectionSub { color:#5a6490; }
QTableWidget {
    background:#111526; color:#c8d0ed; gridline-color:#252b52;
    border:1px solid #252b52; border-radius:10px;
}
QHeaderView::section {
    background:#1a1f38; color:#4f86f7; border:none; padding:8px 10px;
    font-weight:700;
}
QTableWidget::item { padding:6px; }
QTableWidget::item:selected { background:#1e2445; }
QLabel#emptyMsg { color:#2e3860; font-weight:700; }
)";

static const char* kResultsSSLight = R"(
QWidget#resultsRoot { background:#f4f6fb; }
QWidget#summaryBar {
    background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #f4f6fb);
    border-bottom:1px solid #dde2f0;
}
QWidget#sumCard {
    background:#ffffff; border-radius:10px; border:1px solid #dde2f0;
    padding:14px 22px;
}
QLabel#sumTitle {
    color:#8892b8; font-weight:700;
    letter-spacing:1px; background:transparent;
}
QLabel#sumValue { font-weight:900; background:transparent; }
QToolButton#hiddenChartsBtn, QToolButton#monthSelectBtn, QToolButton#pageModeBtn {
    background:#ffffff; color:#1e2340; border:1px solid #dde2f0;
    border-radius:8px; padding:8px 14px; font-weight:700;
}
QToolButton#hiddenChartsBtn:hover, QToolButton#monthSelectBtn:hover, QToolButton#pageModeBtn:hover { background:#eef0fa; }
QWidget#gridContainer { background:#f4f6fb; }
QWidget#reportSection, QWidget#pageBreakSection, QWidget#chartsSection { background:transparent; }
QFrame#pageBreakLine { background:#dde2f0; min-height:1px; max-height:1px; border:none; }
QLabel#pageBreakLabel { color:#8892b8; font-weight:800; letter-spacing:1px; background:transparent; }
QLabel#sectionTitle { color:#1e2340; font-weight:800; }
QLabel#sectionSub { color:#6b7280; }
QTableWidget {
    background:#ffffff; color:#1e2340; gridline-color:#dde2f0;
    border:1px solid #dde2f0; border-radius:10px;
}
QHeaderView::section {
    background:#f6f8fe; color:#4f86f7; border:none; padding:8px 10px;
    font-weight:700;
}
QTableWidget::item { padding:6px; }
QTableWidget::item:selected { background:#eef0fa; }
QLabel#emptyMsg { color:#8aa0c8; font-weight:700; }
)";

static QString money(double v)
{
    return QString("$%L1").arg(v, 0, 'f', 0);
}

static void setTableRow(QTableWidget* t, int row, const QString& month, double net, double cogs, double profit)
{
    auto mkItem = [](const QString& text, const QColor& color = {}) {
        auto* it = new QTableWidgetItem(text);
        if (color.isValid()) it->setForeground(color);
        it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        return it;
    };

    t->setItem(row, 0, mkItem(month));
    t->setItem(row, 1, mkItem(money(net), net >= 0 ? QColor("#3ecf8e") : QColor("#e05c6a")));
    t->setItem(row, 2, mkItem(money(cogs), QColor("#f0a500")));
    t->setItem(row, 3, mkItem(money(profit), profit >= 0 ? QColor("#3ecf8e") : QColor("#e05c6a")));
}

static void applyChartAxes(QChart* chart, const QStringList& labels)
{
    auto* axisX = new QBarCategoryAxis;
    axisX->append(labels);
    axisX->setLabelsFont(QFont("Segoe UI", 8));
    axisX->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisX->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    chart->addAxis(axisX, Qt::AlignBottom);
}

static QStringList monthlyLabels()
{
    return monthNames();
}

static QString pageModeText(bool landscape)
{
    return landscape ? T("Landscape", "أفقي") : T("Portrait", "عمودي");
}

class SafeChartView : public QChartView
{
public:
    using QChartView::QChartView;
protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e && e->button() == Qt::RightButton) {
            e->accept();
            return;
        }
        QChartView::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        if (e && e->button() == Qt::RightButton) {
            e->accept();
            return;
        }
        QChartView::mouseReleaseEvent(e);
    }
};

static QChartView* makeChartView(QChart* chart, bool zoomable = true)
{
    auto* view = new SafeChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setRubberBand(zoomable ? QChartView::RectangleRubberBand : QChartView::NoRubberBand);
    return view;
}


static const char* kMonthCardSSDark = R"(
QFrame#monthCard {
    background:#1a1f38;
    border:1px solid #252b52;
    border-radius:12px;
}
QFrame#monthCard[highlighted="true"] {
    border:2px solid #4f86f7;
}
QWidget#monthHandle {
    background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #1e264a, stop:1 #1a1f38);
    border-bottom:1px solid #252b52;
    border-radius:12px 12px 0 0;
}
QLabel#monthTitle {
    color:#c8d0ed;
    font-weight:800;
    background:transparent;
}
QLabel#monthHint {
    color:#5a6490;
    font-weight:700;
    background:transparent;
}
QFrame#metricBox {
    background:#111526;
    border:1px solid #252b52;
    border-radius:10px;
}
QLabel#metricLabel {
    color:#5a6490;
    font-weight:700;
    background:transparent;
}
QLabel#metricValue {
    color:#c8d0ed;
    font-weight:900;
    background:transparent;
}
)";

static const char* kMonthCardSSLight = R"(
QFrame#monthCard {
    background:#ffffff;
    border:1px solid #dde2f0;
    border-radius:12px;
}
QFrame#monthCard[highlighted="true"] {
    border:2px solid #4f86f7;
}
QWidget#monthHandle {
    background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #eef0fa, stop:1 #ffffff);
    border-bottom:1px solid #dde2f0;
    border-radius:12px 12px 0 0;
}
QLabel#monthTitle {
    color:#1e2340;
    font-weight:800;
    background:transparent;
}
QLabel#monthHint {
    color:#8aa0c8;
    font-weight:700;
    background:transparent;
}
QFrame#metricBox {
    background:#f6f8fe;
    border:1px solid #dde2f0;
    border-radius:10px;
}
QLabel#metricLabel {
    color:#8892b8;
    font-weight:700;
    background:transparent;
}
QLabel#metricValue {
    color:#1e2340;
    font-weight:900;
    background:transparent;
}
)";

class MonthReportCard : public QFrame
{
    Q_OBJECT
public:
    MonthReportCard(const QString& month,
                    double netSales,
                    double cogs,
                    double profit,
                    const QColor& accent,
                    QWidget* parent = nullptr)
        : QFrame(parent), m_month(month)
    {
        setObjectName("monthCard");
        setAcceptDrops(true);
        setCursor(Qt::OpenHandCursor);
        setStyleSheet(g_lightMode ? kMonthCardSSLight : kMonthCardSSDark);
        setFixedHeight(168);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        m_handle = new QWidget;
        m_handle->setObjectName("monthHandle");
        m_handle->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        auto* hl = new QHBoxLayout(m_handle);
        hl->setContentsMargins(14, 10, 14, 10);
        hl->setSpacing(10);

        auto* dot = new QLabel("◆");
        dot->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        dot->setStyleSheet(QString("color:%1;background:transparent;font-weight:900;").arg(accent.name()));
        m_title = new QLabel(month.toUpper());
        m_title->setObjectName("monthTitle");
        m_title->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        auto* hint = new QLabel("⋮⋮");
        hint->setObjectName("monthHint");
        hint->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        hint->setStyleSheet(QString("color:%1;").arg(accent.name()));

        hl->addWidget(dot);
        hl->addWidget(m_title);
        hl->addStretch();
        hl->addWidget(hint);
        root->addWidget(m_handle);

        auto* grid = new QGridLayout;
        grid->setContentsMargins(12, 12, 12, 12);
        grid->setHorizontalSpacing(10);
        grid->setVerticalSpacing(10);

        auto makeMetric = [&](int row, int col, const QString& label, const QString& value, const QColor& valueColor) {
            auto* box = new QFrame;
            box->setObjectName("metricBox");
            box->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            auto* bl = new QVBoxLayout(box);
            bl->setContentsMargins(10, 8, 10, 8);
            bl->setSpacing(4);
            auto* lbl = new QLabel(label);
            lbl->setObjectName("metricLabel");
            lbl->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            auto* val = new QLabel(value);
            val->setObjectName("metricValue");
            val->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            val->setStyleSheet(QString("color:%1;background:transparent;font-weight:900;").arg(valueColor.name()));
            bl->addWidget(lbl);
            bl->addWidget(val);
            grid->addWidget(box, row, col);
        };

        makeMetric(0, 0, T("Net Sales", "صافي المبيعات"), money(netSales),
                   netSales >= 0 ? QColor("#3ecf8e") : QColor("#e05c6a"));
        makeMetric(0, 1, T("COGS", "تكلفة البضاعة"), money(cogs), QColor("#f0a500"));
        makeMetric(1, 0, T("Profit Margin", "هامش الربح"), money(profit),
                   profit >= 0 ? QColor("#3ecf8e") : QColor("#e05c6a"));

        // Spacer box to balance grid
        auto* spacer = new QFrame;
        spacer->setObjectName("metricBox");
        spacer->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        spacer->setStyleSheet("background:transparent;border:none;");
        grid->addWidget(spacer, 1, 1);

        root->addLayout(grid);

        // Make the whole card draggable by disabling child hit-testing.
        for (auto* child : findChildren<QWidget*>()) {
            child->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        }
    }

    void setCardIndex(int i) { m_index = i; }
    void setFlowIndex(int i) { m_flowIndex = i; }
    int cardIndex() const { return m_index; }
    QString month() const { return m_month; }

signals:
    void swapRequested(int fromIdx, int toIdx);
    void insertSeparatorRequested(int afterFlowIndex);

protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e && e->button() == Qt::LeftButton) {
            m_dragStart = e->pos();
            e->accept();
            return;
        }
        QFrame::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if (!e || !(e->buttons() & Qt::LeftButton)) {
            QFrame::mouseMoveEvent(e);
            return;
        }
        if ((e->pos() - m_dragStart).manhattanLength() < QApplication::startDragDistance()) {
            return;
        }
        auto* mime = new QMimeData;
        mime->setData("application/x-account-flow-item", QByteArray::number(m_flowIndex));
        auto* drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->exec(Qt::MoveAction);
    }

    void dragEnterEvent(QDragEnterEvent* e) override
    {
        if (e && e->mimeData() && e->mimeData()->hasFormat("application/x-account-flow-item")) {
            e->acceptProposedAction();
            return;
        }
        QFrame::dragEnterEvent(e);
    }

    void dragMoveEvent(QDragMoveEvent* e) override
    {
        if (e && e->mimeData() && e->mimeData()->hasFormat("application/x-account-flow-item")) {
            e->acceptProposedAction();
            return;
        }
        QFrame::dragMoveEvent(e);
    }

    void dropEvent(QDropEvent* e) override
    {
        if (!e || !e->mimeData() || !e->mimeData()->hasFormat("application/x-account-flow-item")) {
            QFrame::dropEvent(e);
            return;
        }
        const int from = QString::fromUtf8(e->mimeData()->data("application/x-account-flow-item")).toInt();
        if (from != m_flowIndex) emit swapRequested(from, m_flowIndex);
        e->acceptProposedAction();
    }

    void contextMenuEvent(QContextMenuEvent* e) override
    {
        QMenu menu(this);
        QAction* insertSep = menu.addAction(T("Add page separator below", "إضافة فاصل صفحة أسفلها"));
        if (menu.exec(e->globalPos()) == insertSep) {
            emit insertSeparatorRequested(m_flowIndex);
        }
    }

private:
    int m_index{0};
    int m_flowIndex{0};
    QPoint m_dragStart;
    QWidget* m_handle{nullptr};
    QLabel* m_title{nullptr};
    QString m_month;
};

class PageSeparatorCard : public QFrame
{
    Q_OBJECT
public:
    explicit PageSeparatorCard(QWidget* parent = nullptr) : QFrame(parent)
    {
        setObjectName("pageSeparatorCard");
        setAcceptDrops(true);
        setCursor(Qt::OpenHandCursor);
        setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
        setFixedHeight(44);

        auto* root = new QHBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        auto* left = new QFrame;
        left->setObjectName("pageBreakLine");
        left->setFrameShape(QFrame::HLine);
        left->setFrameShadow(QFrame::Plain);
        left->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* right = new QFrame;
        right->setObjectName("pageBreakLine");
        right->setFrameShape(QFrame::HLine);
        right->setFrameShadow(QFrame::Plain);
        right->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_label = new QLabel(T("Page separator", "فاصل صفحة"));
        m_label->setObjectName("pageBreakLabel");
        m_label->setAlignment(Qt::AlignCenter);

        root->addWidget(left, 1);
        root->addWidget(m_label);
        root->addWidget(right, 1);
    }

    void setFlowIndex(int i) { m_flowIndex = i; }
    void setSeparatorId(int id) { m_separatorId = id; }

signals:
    void swapRequested(int fromIdx, int toIdx);
    void removeRequested(int separatorId);

protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e && e->button() == Qt::LeftButton) {
            m_dragStart = e->pos();
            e->accept();
            return;
        }
        QFrame::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if (!e || !(e->buttons() & Qt::LeftButton)) {
            QFrame::mouseMoveEvent(e);
            return;
        }
        if ((e->pos() - m_dragStart).manhattanLength() < QApplication::startDragDistance()) return;
        auto* mime = new QMimeData;
        mime->setData("application/x-account-flow-item", QByteArray::number(m_flowIndex));
        auto* drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->exec(Qt::MoveAction);
    }

    void dragEnterEvent(QDragEnterEvent* e) override
    {
        if (e && e->mimeData() && e->mimeData()->hasFormat("application/x-account-flow-item")) {
            e->acceptProposedAction();
            return;
        }
        QFrame::dragEnterEvent(e);
    }

    void dragMoveEvent(QDragMoveEvent* e) override
    {
        if (e && e->mimeData() && e->mimeData()->hasFormat("application/x-account-flow-item")) {
            e->acceptProposedAction();
            return;
        }
        QFrame::dragMoveEvent(e);
    }

    void dropEvent(QDropEvent* e) override
    {
        if (!e || !e->mimeData() || !e->mimeData()->hasFormat("application/x-account-flow-item")) {
            QFrame::dropEvent(e);
            return;
        }
        const int from = QString::fromUtf8(e->mimeData()->data("application/x-account-flow-item")).toInt();
        if (from != m_flowIndex) emit swapRequested(from, m_flowIndex);
        e->acceptProposedAction();
    }

    void contextMenuEvent(QContextMenuEvent* e) override
    {
        QMenu menu(this);
        QAction* removeAct = menu.addAction(T("Remove page separator", "إزالة فاصل الصفحة"));
        if (menu.exec(e->globalPos()) == removeAct) emit removeRequested(m_separatorId);
    }

private:
    int m_flowIndex{0};
    int m_separatorId{-1};
    QPoint m_dragStart;
    QLabel* m_label{nullptr};
};


ResultsWidget::ResultsWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("resultsRoot");
    setStyleSheet(kResultsSSDark);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_summaryBar = new QWidget;
    m_summaryBar->setObjectName("summaryBar");
    m_summaryBar->setFixedHeight(104);
    auto* bl = new QHBoxLayout(m_summaryBar);
    bl->setContentsMargins(24, 12, 24, 12);
    bl->setSpacing(16);

    auto makeCard = [&](QLabel*& val, const QString& txt, const QColor& col) {
        auto* card = new QWidget;
        card->setObjectName("sumCard");
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(14, 10, 14, 10);
        cl->setSpacing(4);
        auto* title = new QLabel(txt);
        title->setObjectName("sumTitle");
        val = new QLabel("—");
        val->setObjectName("sumValue");
        val->setStyleSheet(QString("color:%1;background:transparent;").arg(col.name()));
        cl->addWidget(title);
        cl->addWidget(val);
        bl->addWidget(card, 1);
    };

    makeCard(m_sumNetSales, T("NET SALES", "\u0635\u0627\u0641\u064A \u0627\u0644\u0645\u0628\u064A\u0639\u0627\u062A"), QColor("#3ecf8e"));
    makeCard(m_sumCOGS, T("COGS", "\u062A\u0643\u0644\u0641\u0629 \u0627\u0644\u0628\u0636\u0627\u0639\u0629"), QColor("#f0a500"));
    makeCard(m_sumProfit, T("PROFIT MARGIN", "\u0647\u0627\u0645\u0634 \u0627\u0644\u0631\u0628\u062D"), QColor("#4f86f7"));

    m_hiddenBtn = new QToolButton;
    m_hiddenBtn->setObjectName("hiddenChartsBtn");
    m_hiddenBtn->setText(T("Hidden charts", "\u0627\u0644\u0631\u0633\u0648\u0645 \u0627\u0644\u0645\u062E\u0641\u064A\u0629"));
    m_hiddenBtn->setPopupMode(QToolButton::InstantPopup);
    m_hiddenMenu = new QMenu(m_hiddenBtn);
    connect(m_hiddenMenu, &QMenu::triggered, this, &ResultsWidget::onRestoreHidden);
    m_hiddenBtn->setMenu(m_hiddenMenu);
    bl->addWidget(m_hiddenBtn, 0, Qt::AlignVCenter);

    m_monthBtn = new QToolButton;
    m_monthBtn->setObjectName("monthSelectBtn");
    m_monthBtn->setPopupMode(QToolButton::InstantPopup);
    m_monthBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_monthBtn->setArrowType(Qt::DownArrow);
    m_monthMenu = new StayOpenMenu(m_monthBtn);
    m_monthBtn->setMenu(m_monthMenu);
    bl->addWidget(m_monthBtn, 0, Qt::AlignVCenter);

    m_orientBtn = new QToolButton;
    m_orientBtn->setObjectName("pageModeBtn");
    m_orientBtn->setPopupMode(QToolButton::InstantPopup);
    m_orientBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_orientBtn->setArrowType(Qt::DownArrow);
    m_orientMenu = new QMenu(m_orientBtn);
    QAction* landscapeAct = m_orientMenu->addAction(T("Landscape", "أفقي"));
    landscapeAct->setData(true);
    QAction* portraitAct = m_orientMenu->addAction(T("Portrait", "عمودي"));
    portraitAct->setData(false);
    connect(m_orientMenu, &QMenu::triggered, this, [this](QAction* act) {
        if (!act) return;
        m_pageLandscape = act->data().toBool();
        updatePageMode();
    });
    m_orientBtn->setMenu(m_orientMenu);
    bl->addWidget(m_orientBtn, 0, Qt::AlignVCenter);

    root->addWidget(m_summaryBar);

    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setStyleSheet("QScrollArea{background:transparent;}QScrollArea QWidget{background:transparent;}");

    m_container = new QWidget;
    m_container->setObjectName("gridContainer");
    m_contentLayout = new QVBoxLayout(m_container);
    m_contentLayout->setContentsMargins(20, 20, 20, 20);
    m_contentLayout->setSpacing(18);
    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll, 1);

    rebuildHiddenMenu();
    rebuildMonthSelectorMenu();
    updatePageMode();
}


void ResultsWidget::appendChart(const AppData& data, const ChartRequest& request)
{
    if (!m_flowSection || !m_flowLayout)
        return;

    QChartView* view = createChartView(data, request);
    if (!view)
        return;

    addCard(request, view);

    ResultFlowItem item;
    item.kind = ResultFlowItemKind::ChartCard;
    item.index = m_cards.isEmpty() ? -1 : m_cards.last()->cardIndex();
    m_flowOrder.append(item);

    rebuildFlow();
    rebuildHiddenMenu();
}

void ResultsWidget::clearResults()
{
    if (m_flowSection) {
        m_contentLayout->removeWidget(m_flowSection);
        m_flowSection->deleteLater();
        m_flowSection = nullptr;
        m_flowLayout = nullptr;
        m_flowEmpty = nullptr;
    }

    if (m_monthSection) {
        m_contentLayout->removeWidget(m_monthSection);
        m_monthSection->deleteLater();
        m_monthSection = nullptr;
        m_monthGrid = nullptr;
        m_monthEmpty = nullptr;
    }
    for (auto* c : m_monthCards) c->deleteLater();
    m_monthCards.clear();
    m_monthOrder.clear();
    m_visibleMonths.clear();

    if (m_pageBreakSection) {
        m_contentLayout->removeWidget(m_pageBreakSection);
        m_pageBreakSection->deleteLater();
        m_pageBreakSection = nullptr;
    }
    if (m_chartsSection) {
        m_contentLayout->removeWidget(m_chartsSection);
        m_chartsSection->deleteLater();
        m_chartsSection = nullptr;
        m_grid = nullptr;
        m_emptyState = nullptr;
    }
    for (auto* c : m_cards) c->deleteLater();
    for (auto* c : m_hiddenCards) c->deleteLater();
    m_cards.clear();
    m_cardRequests.clear();
    m_hiddenCards.clear();
    m_hiddenRequests.clear();
    m_separatorCards.clear();
    m_flowOrder.clear();
    rebuildHiddenMenu();
}

void ResultsWidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_summaryBar) m_summaryBar->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_pageBreakSection) m_pageBreakSection->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_hiddenBtn) m_hiddenBtn->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_monthBtn) m_monthBtn->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_orientBtn) m_orientBtn->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_scroll) m_scroll->setStyleSheet(g_lightMode
        ? "QScrollArea{background:#f4f6fb;}QScrollArea QWidget{background:transparent;}QScrollBar:vertical{background:#f4f6fb;width:8px;border-radius:4px;}QScrollBar::handle:vertical{background:#c8d0ed;border-radius:4px;min-height:30px;}QScrollBar::handle:vertical:hover{background:#4f86f7;}"
        : "QScrollArea{background:#0d1020;}QScrollArea QWidget{background:transparent;}QScrollBar:vertical{background:#0d1020;width:8px;border-radius:4px;}QScrollBar::handle:vertical{background:#2e3860;border-radius:4px;min-height:30px;}QScrollBar::handle:vertical:hover{background:#4f86f7;}");
    for (auto* card : m_monthCards) {
        if (card) card->setStyleSheet(g_lightMode ? kMonthCardSSLight : kMonthCardSSDark);
    }
}

void ResultsWidget::buildResults(const AppData& data)
{
    setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_hiddenBtn) m_hiddenBtn->setEnabled(false);

    clearResults();

    m_sumNetSales->setText(money(data.totalNetSales));
    m_sumCOGS->setText(money(data.totalCOGS));
    m_sumProfit->setText(money(data.totalProfit));
    m_sumProfit->setStyleSheet(QString("color:%1;font-weight:900;background:transparent;")
                               .arg(data.totalProfit >= 0 ? "#3ecf8e" : "#e05c6a"));

    m_flowSection = new QWidget;
    m_flowSection->setObjectName("flowSection");
    auto* vl = new QVBoxLayout(m_flowSection);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(14);

    auto* title = new QLabel(T("Results page", "صفحة النتائج"));
    title->setObjectName("sectionTitle");
    auto* sub = new QLabel(T("Choose one or more month cards, then place them before or after the charts.", "اختر بطاقة شهر واحدة أو أكثر ثم ضعها قبل المخططات أو بعدها."));
    sub->setObjectName("sectionSub");
    vl->addWidget(title);
    vl->addWidget(sub);

    m_flowLayout = new QVBoxLayout;
    m_flowLayout->setContentsMargins(0, 0, 0, 0);
    m_flowLayout->setSpacing(16);
    vl->addLayout(m_flowLayout);
    m_contentLayout->addWidget(m_flowSection);

    m_monthCards.clear();
    m_monthOrder.clear();
    m_cards.clear();
    m_cardRequests.clear();
    m_separatorCards.clear();
    m_flowOrder.clear();
    m_nextSeparatorId = 0;

    static const QColor kMonthAccents[] = {
        QColor("#4f86f7"), QColor("#f0a500"), QColor("#e05c6a"), QColor("#3ecf8e"),
        QColor("#9b6cf9"), QColor("#62c4e3"), QColor("#ff9f43"), QColor("#b0e96a"),
        QColor("#fd79a8"), QColor("#00cec9"), QColor("#4f86f7"), QColor("#f0a500"),
        QColor("#3ecf8e")
    };

    m_monthCards.append(new MonthReportCard(
        T("All months", "كل الأشهر"),
        data.totalNetSales,
        data.totalCOGS,
        data.totalProfit,
        kMonthAccents[0],
        m_container));
    m_monthCards.last()->setCardIndex(0);
    m_monthCards.last()->setFlowIndex(0);
    m_monthOrder.append(0);

    const auto months = monthNames();
    for (int i = 0; i < 12; ++i) {
        auto* card = new MonthReportCard(
            months.value(i),
            data.netSales[i],
            data.cogs[i],
            data.profitMargin[i],
            kMonthAccents[(i + 1) % 13],
            m_container);
        card->setCardIndex(i + 1);
        card->setFlowIndex(i + 1);
        m_monthCards.append(card);
        m_monthOrder.append(i + 1);
    }

    m_visibleMonths.clear();
    for (const auto& item : data.resultFlowOrder) {
        if (item.kind == ResultFlowItemKind::MonthCard && item.index >= 0 && item.index <= 12 && !m_visibleMonths.contains(item.index))
            m_visibleMonths << item.index;
    }
    // NOTE: intentionally allow empty — means no month cards shown by default
    // User selects which months to display via the Months dropdown

    for (const auto& req : data.chartRequests) {
        QChartView* view = createChartView(data, req);
        if (!view) continue;
        addCard(req, view);
    }

    rebuildMonthSelectorMenu();
    ensureDefaultFlowOrder();
    updatePageMode();
    rebuildFlow();
    rebuildHiddenMenu();
}


QWidget* ResultsWidget::buildReportSection(const AppData& data)
{
    auto* section = new QWidget;
    section->setObjectName("reportSection");
    auto* vl = new QVBoxLayout(section);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(16);

    auto* title = new QLabel(T("Monthly Report", "التقرير الشهري"));
    title->setObjectName("sectionTitle");
    auto* sub = new QLabel(T(
        "Each month appears as a draggable summary card with net sales, COGS, and profit margin.",
        "كل شهر يظهر كبطاقة ملخص قابلة للسحب مع صافي المبيعات وتكلفة البضاعة وهامش الربح"));
    sub->setObjectName("sectionSub");
    vl->addWidget(title);
    vl->addWidget(sub);

    m_monthGrid = new QGridLayout;
    m_monthGrid->setContentsMargins(0, 0, 0, 0);
    m_monthGrid->setSpacing(16);
    vl->addLayout(m_monthGrid);

    m_monthCards.clear();
    m_monthOrder.clear();

    static const QColor kMonthAccents[] = {
        QColor("#4f86f7"), QColor("#f0a500"), QColor("#e05c6a"), QColor("#3ecf8e"),
        QColor("#9b6cf9"), QColor("#62c4e3"), QColor("#ff9f43"), QColor("#b0e96a"),
        QColor("#fd79a8"), QColor("#00cec9"), QColor("#4f86f7"), QColor("#f0a500")
    };

    const auto months = monthNames();
    for (int i = 0; i < 12; ++i) {
        m_monthOrder.append(i);
        auto* card = new MonthReportCard(
            months.value(i),
            data.netSales[i],
            data.cogs[i],
            data.profitMargin[i],
            kMonthAccents[i % 12],
            m_container);
        card->setCardIndex(i);
        connect(card, &MonthReportCard::swapRequested, this, &ResultsWidget::onSwapMonthCards);
        m_monthCards.append(card);
    }

    rebuildMonthGrid();
    return section;
}

QWidget* ResultsWidget::buildPageBreakSection()
{
    auto* section = new QWidget;
    section->setObjectName("pageBreakSection");
    auto* vl = new QVBoxLayout(section);
    vl->setContentsMargins(0, 10, 0, 10);
    vl->setSpacing(8);

    auto* line = new QFrame;
    line->setObjectName("pageBreakLine");
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* label = new QLabel(T("Page break", "فاصل صفحة"));
    label->setObjectName("pageBreakLabel");
    label->setAlignment(Qt::AlignCenter);

    vl->addWidget(line);
    vl->addWidget(label);
    return section;
}

QWidget* ResultsWidget::buildChartsSection()
{
    auto* section = new QWidget;
    section->setObjectName("chartsSection");
    auto* vl = new QVBoxLayout(section);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(12);

    auto* title = new QLabel(T("Charts", "\u0627\u0644\u0631\u0633\u0648\u0645"));
    title->setObjectName("sectionTitle");
    auto* sub = new QLabel(T("Drag to reorder. Right-click a chart to hide it.", "\u0627\u0633\u062D\u0628 \u0644\u0644\u062A\u0631\u062A\u064A\u0628. \u0627\u0646\u0642\u0631 \u0628\u0627\u0644\u0632\u0631 \u0627\u0644\u0623\u064A\u0645\u0646 \u0644\u0625\u062E\u0641\u0627\u0621 \u0627\u0644\u0631\u0633\u0645."));
    sub->setObjectName("sectionSub");
    vl->addWidget(title);
    vl->addWidget(sub);

    auto* container = new QWidget;
    m_grid = new QGridLayout(container);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(16);
    vl->addWidget(container);

    return section;
}


QList<int> ResultsWidget::monthOrder() const
{
    return m_visibleMonths;
}

QList<ResultFlowItem> ResultsWidget::flowOrder() const
{
    QList<ResultFlowItem> list;
    for (const auto& item : m_flowOrder) list << item;
    return list;
}

void ResultsWidget::rebuildMonthSelectorMenu()
{
    if (!m_monthMenu || !m_monthBtn) return;
    m_monthMenu->clear();

    const auto months = monthNames();

    // ── Select All / Deselect All ──────────────────────────────────────────
    auto* selAll   = m_monthMenu->addAction(T("✓  Select All",   "✓  تحديد الكل"));
    auto* deselAll = m_monthMenu->addAction(T("✗  Deselect All", "✗  إلغاء الكل"));
    m_monthMenu->addSeparator();

    // ── All-months summary card (index 0) + individual months (1–12) ──────
    QList<QAction*> acts;
    for (int i = 0; i <= 12; ++i) {
        const QString label = (i == 0)
            ? T("All months (summary)", "كل الأشهر (ملخص)")
            : months.value(i - 1);
        QAction* act = m_monthMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_visibleMonths.contains(i));
        act->setData(i);
        acts << act;
    }

    // Helper: rebuild button label from current check state
    auto updateLabel = [this, acts]() {
        QList<int> sel;
        for (auto* a : acts)
            if (a->isChecked()) sel << a->data().toInt();

        QString txt;
        if (sel.isEmpty()) {
            txt = T("Months: None selected", "الأشهر: لا شيء محدد");
        } else if (sel.size() == 13) {
            txt = T("Months: All", "الأشهر: الكل");
        } else {
            // Build a readable label (skip index-0 "All months" summary in the name list)
            QStringList names;
            for (int x : sel) {
                names << (x == 0 ? T("Summary", "ملخص") : monthNames().value(x - 1));
            }
            txt = (names.size() <= 3)
                ? T("Months: ", "الأشهر: ") + names.join(", ")
                : T("Months: ", "الأشهر: ") + names.mid(0, 3).join(", ")
                  + QStringLiteral(" +%1").arg(names.size() - 3);
        }
        m_monthBtn->setText(txt);
    };

    // Helper: apply checked state → m_visibleMonths → rebuild flow
    auto applySelection = [this, acts, updateLabel]() {
        m_visibleMonths.clear();
        for (auto* a : acts)
            if (a->isChecked()) m_visibleMonths << a->data().toInt();
        updateLabel();
        ensureDefaultFlowOrder();
        rebuildFlow();
    };

    // Select All
    QObject::connect(selAll, &QAction::triggered, m_monthBtn, [acts, applySelection]() {
        for (auto* a : acts) a->setChecked(true);
        applySelection();
    });

    // Deselect All
    QObject::connect(deselAll, &QAction::triggered, m_monthBtn, [acts, applySelection]() {
        for (auto* a : acts) a->setChecked(false);
        applySelection();
    });

    // Individual month toggles
    for (auto* act : acts) {
        QObject::connect(act, &QAction::toggled, m_monthBtn, [applySelection](bool) {
            applySelection();
        });
    }

    // Set initial label
    updateLabel();
}

void ResultsWidget::setVisibleMonths(const QList<int>& months)
{
    m_visibleMonths.clear();
    for (int m : months) {
        if (m < 0 || m > 12) continue;   // 0=summary, 1-12=individual months
        if (!m_visibleMonths.contains(m)) m_visibleMonths << m;
    }
    // Allow empty — shows no month cards; user picks via dropdown
    rebuildMonthSelectorMenu();
    ensureDefaultFlowOrder();
    rebuildFlow();
}

void ResultsWidget::updatePageMode()
{
    const int width = m_pageLandscape ? 1123 : 794;
    if (m_container) {
        m_container->setMinimumWidth(width);
        m_container->setMaximumWidth(width);
    }
    if (m_orientBtn) {
        m_orientBtn->setText(T("Page: %1", "الصفحة: %1").arg(pageModeText(m_pageLandscape)));
    }
}

void ResultsWidget::ensureDefaultFlowOrder()
{
    // m_visibleMonths may be empty — that's valid (no month cards shown)
    // Values: 0 = "All months" summary card, 1-12 = individual month cards

    QSet<int> seenMonths;
    m_flowOrder.clear();
    for (int idx : m_visibleMonths) {
        if (idx < 0 || idx > 12 || seenMonths.contains(idx)) continue;
        seenMonths.insert(idx);
        m_flowOrder.append(ResultFlowItem{ResultFlowItemKind::MonthCard, idx, -1});
    }
    for (int i = 0; i < m_cards.size(); ++i)
        m_flowOrder.append(ResultFlowItem{ResultFlowItemKind::ChartCard, i, -1});
}


void ResultsWidget::rebuildMonthGrid()
{
    if (!m_monthGrid) return;

    while (auto* item = m_monthGrid->takeAt(0)) {
        delete item;
    }

    if (m_monthCards.isEmpty()) {
        if (!m_monthEmpty) {
            m_monthEmpty = new QLabel(T("No months available.", "لا توجد أشهر متاحة"), m_monthSection);
            m_monthEmpty->setObjectName("emptyMsg");
            m_monthEmpty->setAlignment(Qt::AlignCenter);
        }
        m_monthGrid->addWidget(m_monthEmpty, 0, 0, 1, 3);
        return;
    }

    const int cols = 3;
    for (int i = 0; i < m_monthCards.size(); ++i) {
        auto* card = m_monthCards[i];
        if (!card) continue;
        card->setCardIndex(i);
        card->setFixedSize(370, 168);
        card->show();
        m_monthGrid->addWidget(card, i / cols, i % cols);
    }
}

void ResultsWidget::onSwapMonthCards(int fromIdx, int toIdx)
{
    if (fromIdx < 0 || fromIdx >= m_monthCards.size()) return;
    if (toIdx < 0 || toIdx >= m_monthCards.size()) return;
    m_monthCards.swapItemsAt(fromIdx, toIdx);
    m_monthOrder.swapItemsAt(fromIdx, toIdx);
    rebuildMonthGrid();
}

void ResultsWidget::addCard(const ChartRequest& request, QChartView* view)
{
    auto* card = new DraggableChartCard(request.title.isEmpty() ? metricDisplayName(request.metricA) : request.title, view, m_container);
    card->setCardIndex(m_cards.size());
    connect(card, &DraggableChartCard::swapRequested, this, &ResultsWidget::onSwapFlowItems);
    connect(card, &DraggableChartCard::insertSeparatorRequested, this, &ResultsWidget::onAddSeparatorAfter);
    connect(card, &DraggableChartCard::hideRequested, this, &ResultsWidget::onHideCard);
    connect(card, &DraggableChartCard::editRequested, this, [this](int) { emit editChartsRequested(); });
    m_cards.append(card);
    m_cardRequests.append(request);
}

void ResultsWidget::rebuildFlow()
{
    if (!m_flowLayout) return;

    while (auto* item = m_flowLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->hide();
            item->widget()->setParent(nullptr);
        }
        delete item;
    }

    if (m_flowOrder.isEmpty()) {
        if (!m_flowEmpty) {
            m_flowEmpty = new QLabel(T("No results available.", "لا توجد نتائج متاحة"), m_flowSection);
            m_flowEmpty->setObjectName("emptyMsg");
            m_flowEmpty->setAlignment(Qt::AlignCenter);
        }
        m_flowLayout->addWidget(m_flowEmpty);
        return;
    }

    for (int i = 0; i < m_flowOrder.size(); ++i) {
        auto& item = m_flowOrder[i];
        if (item.kind == ResultFlowItemKind::MonthCard) {
            // item.index: 0 = "All months" summary card, 1-12 = individual month cards
            if (item.index < 0 || item.index >= m_monthCards.size()) continue;
            auto* card = m_monthCards[item.index];
            if (!card) continue;
            card->setFlowIndex(i);
            card->setMinimumSize(item.index == 0 ? QSize(480, 168) : QSize(370, 168));
            card->setMaximumHeight(220);
            card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            card->show();
            m_flowLayout->addWidget(card);
        } else if (item.kind == ResultFlowItemKind::ChartCard) {
            DraggableChartCard* card = nullptr;
            for (auto* c : m_cards) {
                if (c && c->cardIndex() == item.index) { card = c; break; }
            }
            if (!card) continue;
            card->setFlowIndex(i);
            card->setMinimumSize(460, 360);
            card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            card->show();
            m_flowLayout->addWidget(card);
        } else {
            PageSeparatorCard* sep = m_separatorCards.value(item.id, nullptr);
            if (!sep) {
                sep = new PageSeparatorCard(m_flowSection);
                sep->setSeparatorId(item.id);
                connect(sep, &PageSeparatorCard::swapRequested, this, &ResultsWidget::onSwapFlowItems);
                connect(sep, &PageSeparatorCard::removeRequested, this, &ResultsWidget::onRemoveSeparator);
                m_separatorCards.insert(item.id, sep);
            }
            sep->setFlowIndex(i);
            sep->show();
            m_flowLayout->addWidget(sep);
        }
    }
}

void ResultsWidget::rebuildGrid()
{
    if (!m_grid) return;
    while (auto* item = m_grid->takeAt(0)) {
        delete item;
    }
    if (m_emptyState) {
        m_emptyState->deleteLater();
        m_emptyState = nullptr;
    }
    const int cols = 2;
    for (int i = 0; i < m_cards.size(); ++i) {
        m_cards[i]->setCardIndex(i);
        m_cards[i]->setFixedSize(460, 360);
        m_cards[i]->show();
        m_grid->addWidget(m_cards[i], i / cols, i % cols);
    }
    if (m_cards.isEmpty()) {
        m_emptyState = new QLabel(T("No charts selected.", "لا توجد رسوم مختارة"), m_container);
        m_emptyState->setObjectName("emptyMsg");
        m_emptyState->setAlignment(Qt::AlignCenter);
        m_grid->addWidget(m_emptyState, 0, 0, 1, 2);
    }
}

void ResultsWidget::rebuildHiddenMenu()
{
    if (!m_hiddenMenu || !m_hiddenBtn) return;
    m_hiddenMenu->clear();
    for (int i = 0; i < m_hiddenCards.size(); ++i) {
        QAction* act = m_hiddenMenu->addAction(m_hiddenCards[i]->title());
        act->setData(i);
    }
    m_hiddenBtn->setEnabled(!m_hiddenCards.isEmpty());
}

void ResultsWidget::onSwapCards(int fromIdx, int toIdx)
{
    if (fromIdx < 0 || fromIdx >= m_cards.size()) return;
    if (toIdx < 0 || toIdx >= m_cards.size()) return;
    m_cards.swapItemsAt(fromIdx, toIdx);
    m_cardRequests.swapItemsAt(fromIdx, toIdx);
    rebuildGrid();
}

void ResultsWidget::onHideCard(int cardIndex)
{
    int pos = -1;
    for (int i = 0; i < m_cards.size(); ++i) {
        if (m_cards[i] && m_cards[i]->cardIndex() == cardIndex) { pos = i; break; }
    }
    if (pos < 0 || pos >= m_cards.size()) return;
    DraggableChartCard* card = m_cards.takeAt(pos);
    ChartRequest req = m_cardRequests.takeAt(pos);
    m_hiddenCards.append(card);
    m_hiddenRequests.append(req);
    for (int i = 0; i < m_flowOrder.size(); ++i) {
        if (m_flowOrder[i].kind == ResultFlowItemKind::ChartCard && m_flowOrder[i].index == cardIndex) {
            m_flowOrder.removeAt(i);
            break;
        }
    }
    card->hide();
    rebuildFlow();
    rebuildHiddenMenu();
}

void ResultsWidget::onRestoreHidden(QAction* action)
{
    if (!action) return;
    const int idx = action->data().toInt();
    if (idx < 0 || idx >= m_hiddenCards.size()) return;
    DraggableChartCard* card = m_hiddenCards.takeAt(idx);
    ChartRequest req = m_hiddenRequests.takeAt(idx);
    m_cards.append(card);
    m_cardRequests.append(req);
    card->show();
    ResultFlowItem item;
    item.kind = ResultFlowItemKind::ChartCard;
    item.index = card->cardIndex();
    m_flowOrder.append(item);
    rebuildFlow();
    rebuildHiddenMenu();
}

void ResultsWidget::onSwapFlowItems(int fromIdx, int toIdx)
{
    if (fromIdx < 0 || fromIdx >= m_flowOrder.size()) return;
    if (toIdx < 0 || toIdx >= m_flowOrder.size()) return;
    m_flowOrder.swapItemsAt(fromIdx, toIdx);
    rebuildFlow();
}

void ResultsWidget::onAddSeparatorAfter(int flowIndex)
{
    if (flowIndex < 0 || flowIndex >= m_flowOrder.size()) return;
    ResultFlowItem sep;
    sep.kind = ResultFlowItemKind::PageSeparator;
    sep.id = ++m_nextSeparatorId;
    m_flowOrder.insert(flowIndex + 1, sep);
    rebuildFlow();
}

void ResultsWidget::onRemoveSeparator(int separatorId)
{
    for (int i = 0; i < m_flowOrder.size(); ++i) {
        if (m_flowOrder[i].kind == ResultFlowItemKind::PageSeparator && m_flowOrder[i].id == separatorId) {
            m_flowOrder.removeAt(i);
            break;
        }
    }
    if (m_separatorCards.contains(separatorId)) {
        auto* w = m_separatorCards.take(separatorId);
        if (w) w->deleteLater();
    }
    rebuildFlow();
}

QList<QChartView*> ResultsWidget::allChartViews() const
{
    QList<QChartView*> list;
    for (auto* c : m_cards) list << c->chartView();
    return list;
}

QList<ChartRequest> ResultsWidget::chartRequests() const
{
    QList<ChartRequest> list;
    for (const auto& item : m_flowOrder) {
        if (item.kind != ResultFlowItemKind::ChartCard || item.index < 0) continue;
        for (int i = 0; i < m_cards.size() && i < m_cardRequests.size(); ++i) {
            if (m_cards[i] && m_cards[i]->cardIndex() == item.index) {
                list << m_cardRequests[i];
                break;
            }
        }
    }
    return list;
}


static void applyThemeToChart(QChart* chart)
{
    if (g_lightMode) {
        chart->setBackgroundBrush(QBrush(QColor("#ffffff")));
        chart->setPlotAreaBackgroundBrush(QBrush(QColor("#f8f9ff")));
        chart->setTitleBrush(QBrush(QColor("#1e2340")));
        chart->legend()->setLabelColor(QColor("#5a6490"));
    } else {
        chart->setBackgroundBrush(QBrush(QColor("#151929")));
        chart->setPlotAreaBackgroundBrush(QBrush(QColor("#0f1320")));
        chart->setTitleBrush(QBrush(QColor("#c8d0ed")));
        chart->legend()->setLabelColor(QColor("#8892b8"));
    }
    chart->setPlotAreaBackgroundVisible(true);
    chart->setTitleFont(QFont("Segoe UI", 12, QFont::Bold));
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setFont(QFont("Segoe UI", 9));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setMargins(QMargins(4, 4, 4, 4));
}

void ResultsWidget::applyChartTheme(QChart* chart, const QString& title)
{
    if (!chart) return;
    applyThemeToChart(chart);
    chart->setTitle(title);
}

QChartView* ResultsWidget::makePieChart(const QString& title,
                                        const QStringList& labels,
                                        const QList<double>& values)
{
    auto* series = new QPieSeries;
    double total = 0;
    for (double v : values) total += qAbs(v);
    if (total < 0.001) total = 1;

    QColor borderCol = g_lightMode ? QColor("#f4f6fb") : QColor("#0d1020");
    for (int i = 0; i < labels.size() && i < values.size(); ++i) {
        const double v = qAbs(values[i]);
        if (v < 0.001) continue;
        auto* sl = series->append(labels[i], v);
        sl->setColor(kPal[i % kPal.size()]);
        sl->setBorderColor(borderCol);
        sl->setLabelVisible(true);
        sl->setLabel(QString("%1%").arg(v / total * 100.0, 0, 'f', 1));
        sl->setLabelPosition(QPieSlice::LabelOutside);
        sl->setLabelArmLengthFactor(0.18);
        sl->setLabelColor(g_lightMode ? QColor("#1e2340") : QColor("#ffffff"));
        QObject::connect(sl, &QPieSlice::hovered, sl, [sl](bool on) {
            sl->setExploded(on);
        });
    }

    series->setPieSize(0.72);
    auto* chart = new QChart;
    chart->addSeries(series);
    chart->legend()->setVisible(true);
    applyThemeToChart(chart);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
    chart->setTitle(title);

    const auto markers = chart->legend()->markers(series);
    for (int i = 0; i < markers.size() && i < labels.size(); ++i) {
        if (markers[i])
            markers[i]->setLabel(labels[i]);
    }

    return makeChartView(chart, false);
}

QChartView* ResultsWidget::makeCandleChart(const QString& title,
                                           const QStringList& labels,
                                           const QList<double>& values)
{
    auto* series = new QCandlestickSeries;
    series->setIncreasingColor(QColor("#3ecf8e"));
    series->setDecreasingColor(QColor("#e05c6a"));
    series->setBodyOutlineVisible(false);

    for (int i = 0; i < values.size(); ++i) {
        const double cur = values[i];
        const double open = 0.0;
        const double close = cur;
        double hi = qMax(open, close) * 1.02;
        double lo = qMin(open, close) * 0.98;
        if (hi <= lo + 0.001) hi = lo + 1;
        auto* cs = new QCandlestickSet(open, hi, lo, close, static_cast<qreal>(i));
        series->append(cs);
    }

    auto* chart = new QChart;
    chart->addSeries(series);
    applyThemeToChart(chart);
    chart->setTitle(title);

    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    series->attachAxis(axX);
    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    axY->setLabelFormat("$%'i");
    double maxV = 0.0;
    for (double v : values) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    axY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axY);

    // Add invisible dummy line series so the legend shows green/red key
    auto* legInc = new QLineSeries;
    legInc->setName(T("↗ Increasing", "↗ ارتفاع"));
    legInc->setColor(QColor("#3ecf8e"));
    auto* legDec = new QLineSeries;
    legDec->setName(T("↘ Decreasing", "↘ انخفاض"));
    legDec->setColor(QColor("#e05c6a"));
    chart->addSeries(legInc);
    chart->addSeries(legDec);
    legInc->attachAxis(axX);
    legInc->attachAxis(axY);
    legDec->attachAxis(axX);
    legDec->attachAxis(axY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : values) vl << x;
    view->setProperty("chartValues", vl);
    view->setProperty("chartType", "candle");
    view->setProperty("chartTitle", title);
    return view;
}

QChartView* ResultsWidget::makeRankedBarChart(const QString& title,
                                              const QStringList& labels,
                                              const QList<double>& values)
{
    auto* set = new QBarSet(title);
    set->setColor(QColor("#4f86f7"));
    set->setBorderColor(Qt::transparent);
    for (double v : values) *set << v;

    auto* series = new QBarSeries;
    series->append(set);
    series->setBarWidth(0.7);

    auto* chart = new QChart;
    chart->addSeries(series);
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    series->attachAxis(axX);
    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : values) vl << x;
    view->setProperty("chartValues", vl);
    view->setProperty("chartType", "rankedbar");
    view->setProperty("chartTitle", title);
    return view;
}

QChartView* ResultsWidget::makeSingleLineChart(const QString& title,
                                               const QStringList& labels,
                                               const QList<double>& values)
{
    auto* line = new QLineSeries;
    line->setName(title);
    line->setColor(QColor("#4f86f7"));
    for (int i = 0; i < values.size(); ++i)
        line->append(i, values[i]);

    auto* chart = new QChart;
    chart->addSeries(line);
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(false);

    auto* axisX = new QCategoryAxis;
    for (int i = 0; i < labels.size(); ++i)
        axisX->append(labels[i], i);
    axisX->setRange(0, qMax(0, labels.size() - 1));
    axisX->setLabelsFont(QFont("Segoe UI", 8));
    axisX->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisX->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    chart->addAxis(axisX, Qt::AlignBottom);
    line->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setLabelsFont(QFont("Segoe UI", 8));
    axisY->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisY->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    double maxV = 0.0;
    for (double v : values) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    axisY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axisY, Qt::AlignLeft);
    line->attachAxis(axisY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : values) vl << x;
    view->setProperty("chartValues", vl);
    view->setProperty("chartType", "line");
    view->setProperty("chartTitle", title);
    return view;
}

QChartView* ResultsWidget::makeCompareCandleChart(const QString& title,
                                                  const QStringList& labels,
                                                  const QList<double>& seriesA,
                                                  const QList<double>& seriesB,
                                                  const QString& nameA,
                                                  const QString& nameB)
{
    auto* candA = new QCandlestickSeries;
    auto* candB = new QCandlestickSeries;
    candA->setName(nameA);
    candB->setName(nameB);
    candA->setIncreasingColor(QColor("#3ecf8e"));
    candA->setDecreasingColor(QColor("#e05c6a"));
    candB->setIncreasingColor(QColor("#f0a500"));
    candB->setDecreasingColor(QColor("#7cc4ff"));
    candA->setBodyOutlineVisible(false);
    candB->setBodyOutlineVisible(false);

    const int n = qMax(seriesA.size(), seriesB.size());
    for (int i = 0; i < n; ++i) {
        const double a = i < seriesA.size() ? seriesA[i] : 0.0;
        const double b = i < seriesB.size() ? seriesB[i] : 0.0;
        double hiA = qMax(0.0, a) * 1.02;
        double loA = qMin(0.0, a) * 0.98;
        if (hiA <= loA + 0.001) hiA = loA + 1;
        double hiB = qMax(0.0, b) * 1.02;
        double loB = qMin(0.0, b) * 0.98;
        if (hiB <= loB + 0.001) hiB = loB + 1;
        candA->append(new QCandlestickSet(0.0, hiA, loA, a, static_cast<qreal>(i) - 0.18));
        candB->append(new QCandlestickSet(0.0, hiB, loB, b, static_cast<qreal>(i) + 0.18));
    }

    auto* chart = new QChart;
    chart->addSeries(candA);
    chart->addSeries(candB);
    applyChartTheme(chart, title);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);

    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    candA->attachAxis(axX);
    candB->attachAxis(axX);

    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    double maxV = 0.0;
    for (double v : seriesA) maxV = qMax(maxV, qAbs(v));
    for (double v : seriesB) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    axY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axY, Qt::AlignLeft);
    candA->attachAxis(axY);
    candB->attachAxis(axY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : seriesA) vl << x;
    view->setProperty("chartValues", vl);
    view->setProperty("chartType", "comparecandle");
    view->setProperty("chartTitle", title);
    view->setProperty("chartLabels2", labels);
    QVariantList vl2; for (double x : seriesB) vl2 << x;
    view->setProperty("chartValues2", vl2);
    view->setProperty("chartSeriesA", nameA);
    view->setProperty("chartSeriesB", nameB);
    return view;
}

QChartView* ResultsWidget::makeCompareBarChart(const QString& title,
                                               const QStringList& labels,
                                               const QList<double>& seriesA,
                                               const QList<double>& seriesB,
                                               const QString& nameA,
                                               const QString& nameB)
{
    auto* setA = new QBarSet(nameA);
    auto* setB = new QBarSet(nameB);
    setA->setColor(QColor("#f0a500"));
    setB->setColor(QColor("#3ecf8e"));
    setA->setBorderColor(Qt::transparent);
    setB->setBorderColor(Qt::transparent);
    for (double v : seriesA) *setA << v;
    for (double v : seriesB) *setB << v;

    auto* series = new QBarSeries;
    series->append(setA);
    series->append(setB);
    series->setBarWidth(0.65);

    auto* chart = new QChart;
    chart->addSeries(series);
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(true);

    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    series->attachAxis(axX);
    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : seriesA) vl << x;
    view->setProperty("chartValues", vl);
    view->setProperty("chartType", "barcompare");
    view->setProperty("chartTitle", title);
    view->setProperty("chartLabels2", labels);
    QVariantList vl2; for (double x : seriesB) vl2 << x;
    view->setProperty("chartValues2", vl2);
    view->setProperty("chartSeriesA", nameA);
    view->setProperty("chartSeriesB", nameB);
    return view;
}

QChartView* ResultsWidget::makeCompareLineChart(const QString& title,
                                                const QStringList& labels,
                                                const QList<double>& seriesA,
                                                const QList<double>& seriesB,
                                                const QString& nameA,
                                                const QString& nameB)
{
    auto* lineA = new QLineSeries;
    auto* lineB = new QLineSeries;
    lineA->setName(nameA);
    lineB->setName(nameB);
    lineA->setColor(QColor("#f0a500"));
    lineB->setColor(QColor("#3ecf8e"));

    const int n = qMin(seriesA.size(), seriesB.size());
    for (int i = 0; i < n; ++i) {
        lineA->append(i, seriesA[i]);
        lineB->append(i, seriesB[i]);
    }

    auto* chart = new QChart;
    chart->addSeries(lineA);
    chart->addSeries(lineB);
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(true);

    auto* axisX = new QCategoryAxis;
    for (int i = 0; i < labels.size(); ++i)
        axisX->append(labels[i], i);
    axisX->setRange(0, qMax(0, labels.size() - 1));
    axisX->setLabelsFont(QFont("Segoe UI", 8));
    axisX->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisX->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    chart->addAxis(axisX, Qt::AlignBottom);
    lineA->attachAxis(axisX);
    lineB->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setLabelsFont(QFont("Segoe UI", 8));
    axisY->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisY->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    chart->addAxis(axisY, Qt::AlignLeft);
    lineA->attachAxis(axisY);
    lineB->attachAxis(axisY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : seriesA) vl << x;
    view->setProperty("chartValues", vl);
    view->setProperty("chartType", "linecompare");
    view->setProperty("chartTitle", title);
    view->setProperty("chartLabels2", labels);
    QVariantList vl2; for (double x : seriesB) vl2 << x;
    view->setProperty("chartValues2", vl2);
    view->setProperty("chartSeriesA", nameA);
    view->setProperty("chartSeriesB", nameB);
    return view;
}

QChartView* ResultsWidget::makeComparePieChart(const QString& title,
                                               const QString& nameA,
                                               const QString& nameB,
                                               double valueA,
                                               double valueB)
{
    auto* series = new QPieSeries;
    if (valueA == 0 && valueB == 0) {
        valueA = 1;
        valueB = 1;
    }
    const double total = qMax(0.0001, qAbs(valueA) + qAbs(valueB));
    auto* a = series->append(nameA, qMax(0.0, valueA));
    auto* b = series->append(nameB, qMax(0.0, valueB));
    a->setLabelVisible(true);
    b->setLabelVisible(true);
    a->setLabel(QString("%1%").arg(qAbs(valueA) / total * 100.0, 0, 'f', 1));
    b->setLabel(QString("%1%").arg(qAbs(valueB) / total * 100.0, 0, 'f', 1));
    a->setLabelPosition(QPieSlice::LabelOutside);
    b->setLabelPosition(QPieSlice::LabelOutside);
    a->setLabelArmLengthFactor(0.18);
    b->setLabelArmLengthFactor(0.18);
    a->setLabelColor(g_lightMode ? QColor("#1e2340") : QColor("#ffffff"));
    b->setLabelColor(g_lightMode ? QColor("#1e2340") : QColor("#ffffff"));
    a->setColor(kPal[0]);
    b->setColor(kPal[1]);

    series->setPieSize(0.72);
    auto* chart = new QChart;
    chart->addSeries(series);
    applyChartTheme(chart, title);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
    const auto markers = chart->legend()->markers(series);
    if (markers.size() > 0 && markers[0]) markers[0]->setLabel(nameA);
    if (markers.size() > 1 && markers[1]) markers[1]->setLabel(nameB);
    return makeChartView(chart, false);
}

QChartView* ResultsWidget::createChartView(const AppData& data, const ChartRequest& request)
{
    QStringList labels;
    const QList<int>* months = request.months.isEmpty() ? nullptr : &request.months;
    QList<double> a = metricSeriesValues(data, request.metricA, &labels, months, request.accountFilter);
    QList<double> b;
    QString title = request.title.isEmpty() ? metricDisplayName(request.metricA) : request.title;

    switch (request.kind) {
    case ChartKind::Pie:
        return makePieChart(title, labels, a);
    case ChartKind::Candle:
        if (!request.seriesB.isEmpty()) {
            b = metricSeriesValues(data, request.metricB, &labels, months, request.accountFilter);
            return makeCompareCandleChart(title, labels, a, b,
                                          request.seriesA.isEmpty() ? metricDisplayName(request.metricA) : request.seriesA,
                                          request.seriesB.isEmpty() ? metricDisplayName(request.metricB) : request.seriesB);
        }
        if (request.metricA == M_EXPENSES)
            return makeRankedBarChart(title, labels, a);
        return makeCandleChart(title, labels, a);
    case ChartKind::RankedBar:
    case ChartKind::MetricBar:
        return makeRankedBarChart(title, labels, a);
    case ChartKind::MetricLine:
        return makeSingleLineChart(title, labels, a);
    case ChartKind::CompareBar:
        b = metricSeriesValues(data, request.metricB, &labels, months, request.accountFilter);
        return makeCompareBarChart(title, labels, a, b,
                                   request.seriesA.isEmpty() ? metricDisplayName(request.metricA) : request.seriesA,
                                   request.seriesB.isEmpty() ? metricDisplayName(request.metricB) : request.seriesB);
    case ChartKind::CompareLine:
        b = metricSeriesValues(data, request.metricB, &labels, months, request.accountFilter);
        return makeCompareLineChart(title, labels, a, b,
                                    request.seriesA.isEmpty() ? metricDisplayName(request.metricA) : request.seriesA,
                                    request.seriesB.isEmpty() ? metricDisplayName(request.metricB) : request.seriesB);
    case ChartKind::ComparePie: {
        b = metricSeriesValues(data, request.metricB, &labels, months, request.accountFilter);
        double va = 0.0, vb = 0.0;
        for (double x : a) va += qAbs(x);
        for (double x : b) vb += qAbs(x);
        return makeComparePieChart(title,
                                   request.seriesA.isEmpty() ? metricDisplayName(request.metricA) : request.seriesA,
                                   request.seriesB.isEmpty() ? metricDisplayName(request.metricB) : request.seriesB,
                                   va, vb);
    }
    }
    return nullptr;
}


#include "Resultswidget.moc"
