#include "chartselectiondialog.h"
#include "translations.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QPair>
#include <QComboBox>
#include <QAbstractItemView>
#include <QAction>
#include <QMouseEvent>
#include <algorithm>
#include <type_traits>

// ─── Stay-open menu (doesn't close when clicking checkable items) ─────────────
class StayOpenMenu : public QMenu {
public:
    using QMenu::QMenu;
protected:
    void mouseReleaseEvent(QMouseEvent* e) override {
        QAction* a = activeAction();
        if (a && a->isEnabled()) {
            if (a->isCheckable()) {
                a->trigger();   // toggle – but don't close
            } else {
                a->trigger();   // e.g. Select All / Deselect All
            }
            return;             // skip base class so menu stays open
        }
        QMenu::mouseReleaseEvent(e);
    }
};

static QString monthSummaryText(const QList<int>& months);

// Helper: build a stay-open month menu with Select All / Deselect All
static StayOpenMenu* makeMonthMenu(QToolButton* btn, const QList<int>& preChecked,
                                    bool allChecked, QVector<QAction*>* monthActsOut)
{
    auto* menu = new StayOpenMenu(btn);
    const auto names = monthNames();

    // Select All / Deselect All
    auto* selAll  = menu->addAction(T("✓  Select All",   "✓  تحديد الكل"));
    auto* deselAll= menu->addAction(T("✗  Deselect All", "✗  إلغاء الكل"));
    menu->addSeparator();

    // Month checkboxes
    QVector<QAction*> monthActs;
    for (int i = 0; i < 12; ++i) {
        auto* act = menu->addAction(names.value(i));
        act->setCheckable(true);
        act->setChecked(allChecked || preChecked.contains(i));
        monthActs << act;
    }
    if (monthActsOut) *monthActsOut = monthActs;

    // Connect select/deselect all
    auto updateBtn = [btn, monthActs]() {
        QList<int> sel;
        for (int i = 0; i < monthActs.size(); ++i)
            if (monthActs[i]->isChecked()) sel << i;
        btn->setText(T("Months: %1","الأشهر: %1").arg(monthSummaryText(sel)));
    };

    QObject::connect(selAll,   &QAction::triggered, btn, [monthActs, updateBtn]() {
        for (auto* a : monthActs) a->setChecked(true);
        updateBtn();
    });
    QObject::connect(deselAll, &QAction::triggered, btn, [monthActs, updateBtn]() {
        for (auto* a : monthActs) a->setChecked(false);
        updateBtn();
    });
    for (auto* act : monthActs) {
        QObject::connect(act, &QAction::toggled, btn, updateBtn);
    }

    return menu;
}

static const char* kDialogSS = R"(
QDialog { background:#12162b; }
QLabel#title {
    color:#4f86f7; font-weight:800; padding:4px 0 2px 0;
}
QLabel#subtitle {
    color:#5a6490; padding-bottom:8px;
}
QLabel#section {
    color:#c8d0ed; font-weight:800; padding:6px 0 2px 0;
}
QFrame#row {
    background:#1a1f38;
    border-radius:8px;
    border:1px solid #252b52;
}
QFrame#row:hover { border-color:#4f86f7; }
QLabel#metricName {
    color:#c8d0ed; font-weight:700;
}
QCheckBox {
    color:#8892b8; spacing:8px;
}
QCheckBox::indicator {
    width:18px; height:18px; border:2px solid #3a4470;
    border-radius:5px; background:#252d4a;
}
QCheckBox::indicator:checked {
    background:#4f86f7; border-color:#4f86f7;
}
QToolButton#monthBtn, QPushButton#dupBtn {
    background:#252d4a; color:#c8d0ed;
    border:1px solid #3a4470; border-radius:6px;
    padding:5px 10px; min-height:28px;
    font-weight:700;
}
QToolButton#monthBtn:hover, QPushButton#dupBtn:hover {
    border-color:#4f86f7;
    background:#293252;
}
QToolButton#monthBtn::menu-indicator { image:none; }
QComboBox, QLineEdit {
    background:#252d4a;
    color:#c8d0ed;
    border:1px solid #3a4470;
    border-radius:6px;
    padding:5px 8px;
    min-height:28px;
}
QComboBox::drop-down {
    border:0;
    width:24px;
}
QComboBox QAbstractItemView {
    background:#1a1f38;
    color:#c8d0ed;
    selection-background-color:#4f86f7;
    border:1px solid #252b52;
}
QComboBox QAbstractItemView::item {
    background:#1a1f38;
    color:#c8d0ed;
}
QComboBox QAbstractItemView::item:selected {
    background:#4f86f7;
    color:#ffffff;
}
QPushButton#showBtn {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #4f86f7, stop:1 #2a5cc4);
    color:white; font-weight:800;
    border:none; border-radius:8px; min-height:42px; padding:0 32px;
}
QPushButton#showBtn:hover  { background:#5e91f8; }
QPushButton#showBtn:pressed{ background:#3a6fe0; }
QPushButton#cancelBtn {
    background:#1e2340; color:#8892b8;
    border:1px solid #2e3455;
    border-radius:8px; min-height:42px; padding:0 20px;
}
QPushButton#cancelBtn:hover{ background:#252b50; }
QPushButton#addBtn {
    background:transparent; color:#4f86f7;
    font-weight:700;
    border:2px dashed #2e3860;
    border-radius:8px; min-height:36px; padding:0 20px;
}
QPushButton#addBtn:hover { background:#141829; border-color:#4f86f7; }
QPushButton#removeBtn {
    background:transparent; color:#5a6490;
    border:none; font-weight:700;
    min-width:28px; max-width:28px;
}
QPushButton#removeBtn:hover { color:#e05c6a; }
)";

static const char* kDialogSSLight = R"(
QDialog { background:#f4f6fb; }
QLabel#title {
    color:#4f86f7; font-weight:800; padding:4px 0 2px 0;
}
QLabel#subtitle {
    color:#6b7280; padding-bottom:8px;
}
QLabel#section {
    color:#1e2340; font-weight:800; padding:6px 0 2px 0;
}
QFrame#row {
    background:#ffffff;
    border-radius:8px;
    border:1px solid #dde2f0;
}
QFrame#row:hover { border-color:#4f86f7; }
QLabel#metricName {
    color:#1e2340; font-weight:700;
}
QCheckBox {
    color:#5a6490; spacing:8px;
}
QCheckBox::indicator {
    width:18px; height:18px; border:2px solid #cfd7ea;
    border-radius:5px; background:#ffffff;
}
QCheckBox::indicator:checked {
    background:#4f86f7; border-color:#4f86f7;
}
QToolButton#monthBtn, QPushButton#dupBtn {
    background:#ffffff; color:#1e2340;
    border:1px solid #cfd7ea; border-radius:6px;
    padding:5px 10px; min-height:28px;
    font-weight:700;
}
QToolButton#monthBtn:hover, QPushButton#dupBtn:hover {
    border-color:#4f86f7;
    background:#eef1fb;
}
QToolButton#monthBtn::menu-indicator { image:none; }
QComboBox, QLineEdit {
    background:#ffffff;
    color:#1e2340;
    border:1px solid #cfd7ea;
    border-radius:6px;
    padding:5px 8px;
    min-height:28px;
}
QComboBox::drop-down {
    border:0;
    width:24px;
}
QComboBox QAbstractItemView {
    background:#ffffff;
    color:#1e2340;
    selection-background-color:#eef0fa;
    selection-color:#1e2340;
    border:1px solid #dde2f0;
}
QComboBox QAbstractItemView::item {
    background:#ffffff;
    color:#1e2340;
}
QComboBox QAbstractItemView::item:selected {
    background:#eef0fa;
    color:#1e2340;
}
QPushButton#showBtn {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #4f86f7, stop:1 #2a5cc4);
    color:white; font-weight:800;
    border:none; border-radius:8px; min-height:42px; padding:0 32px;
}
QPushButton#showBtn:hover  { background:#5e91f8; }
QPushButton#showBtn:pressed{ background:#3a6fe0; }
QPushButton#cancelBtn {
    background:#ffffff; color:#1e2340;
    border:1px solid #dde2f0;
    border-radius:8px; min-height:42px; padding:0 20px;
}
QPushButton#cancelBtn:hover{ background:#f2f4fb; }
QPushButton#addBtn {
    background:#ffffff; color:#4f86f7;
    font-weight:700;
    border:2px dashed #c8d4f5;
    border-radius:8px; min-height:36px; padding:0 20px;
}
QPushButton#addBtn:hover { background:#eef1fb; border-color:#4f86f7; }
QPushButton#removeBtn {
    background:transparent; color:#8892b8;
    border:none; font-weight:700;
    min-width:28px; max-width:28px;
}
QPushButton#removeBtn:hover { color:#e05c6a; }
)";

struct MetricDef {
    MetricId id;
    const char* en;
    const char* ar;
    bool pie;
    bool candle;
};

static const MetricDef kMetricDefs[] = {
    { M_SALES,             "Sales",             "المبيعات",              true,  true  },
    { M_SALES_RETURN,      "Sales Return",      "مرتجعات المبيعات",      true,  true  },
    { M_PURCHASES,         "Purchases",         "المشتريات",             true,  true  },
    { M_SUPPLIER_PAYMENTS, "Supplier Payments",  "دفعات الموردين",        false, true  },
    { M_EXPENSES,          "Expenses",          "المصروفات",             false, true  },
    { M_INVENTORY,         "Inventory",         "المخزون",               true,  true  },
    { M_NET_SALES,         "Net Sales",         "صافي المبيعات",         true,  true  },
    { M_COGS,              "COGS",              "تكلفة البضاعة",         true,  true  },
    { M_PROFIT_MARGIN,     "Profit Margin",     "هامش الربح",            true,  true  },
};

static QList<QPair<MetricId, QString>> compareMetrics()
{
    return {
        { M_SALES,             metricDisplayName(M_SALES) },
        { M_SALES_RETURN,      metricDisplayName(M_SALES_RETURN) },
        { M_PURCHASES,         metricDisplayName(M_PURCHASES) },
        { M_SUPPLIER_PAYMENTS, metricDisplayName(M_SUPPLIER_PAYMENTS) },
        { M_EXPENSE_AMOUNT,    metricDisplayName(M_EXPENSE_AMOUNT) },
        { M_INVENTORY_OPENING, metricDisplayName(M_INVENTORY_OPENING) },
        { M_INVENTORY_CLOSING, metricDisplayName(M_INVENTORY_CLOSING) },
        { M_NET_SALES,         metricDisplayName(M_NET_SALES) },
        { M_COGS,              metricDisplayName(M_COGS) },
        { M_PROFIT_MARGIN,     metricDisplayName(M_PROFIT_MARGIN) },
    };
}

static const MetricDef* metricDefForId(MetricId id)
{
    for (const auto& def : kMetricDefs) {
        if (def.id == id)
            return &def;
    }
    return &kMetricDefs[0];
}

static QList<int> normalizedMonths(QList<int> months)
{
    std::sort(months.begin(), months.end());
    months.erase(std::unique(months.begin(), months.end()), months.end());
    return months;
}

static QString monthSummaryText(const QList<int>& months)
{
    const auto names = monthNames();
    QList<int> clean = normalizedMonths(months);
    if (clean.isEmpty() || clean.size() == 12)
        return T("All months", "كل الأشهر");

    QStringList parts;
    for (int idx : clean)
        parts << names.value(idx);
    return parts.join(", ");
}

static void updateMonthButtonText(QToolButton* button, const QVector<QAction*>& actions)
{
    if (!button)
        return;

    QList<int> selected;
    for (int i = 0; i < actions.size(); ++i) {
        if (actions[i] && actions[i]->isChecked())
            selected << i;
    }
    button->setText(T("Months: %1", "الأشهر: %1").arg(monthSummaryText(selected)));
}

static void updateCompareMonthButtonText(QToolButton* button, const QVector<QAction*>& actions)
{
    if (!button)
        return;

    QList<int> selected;
    for (int i = 0; i < actions.size(); ++i) {
        if (actions[i] && actions[i]->isChecked())
            selected << i;
    }
    button->setText(T("Month: %1", "الشهر: %1").arg(monthSummaryText(selected)));
}

static QString stripGeneratedMonthSuffix(const QString& text)
{
    const int open = text.lastIndexOf(QStringLiteral(" ("));
    if (open > 0 && text.endsWith(')'))
        return text.left(open).trimmed();
    return text.trimmed();
}

static bool requestIsCompareKind(ChartKind kind)
{
    return kind == ChartKind::CompareBar || kind == ChartKind::CompareLine || kind == ChartKind::ComparePie;
}

ChartSelectionDialog::ChartSelectionDialog(const AppData& data, QWidget* parent)
    : QDialog(parent)
{
    setStyleSheet(g_lightMode ? kDialogSSLight : kDialogSS);
    setWindowTitle(T("Select Charts", "اختيار المخططات"));
    setMinimumWidth(960);
    setModal(true);
    buildUI(data);
}

void ChartSelectionDialog::buildUI(const AppData& data)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    auto* title = new QLabel(T("Choose what appears in Results", "اختر ما يظهر في النتائج"));
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* sub = new QLabel(T(
        "The report shows each month first, then the charts you select below.",
        "التقرير يعرض كل شهر أولاً ثم المخططات التي تختارها أدناه"));
    sub->setObjectName("subtitle");
    sub->setAlignment(Qt::AlignCenter);
    root->addWidget(sub);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea{background:transparent;}QScrollArea QWidget{background:transparent;}");

    auto* body = new QWidget;
    auto* vl = new QVBoxLayout(body);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(10);

    auto* metricToggle = new QToolButton;
    metricToggle->setObjectName("sectionToggle");
    metricToggle->setCheckable(true);
    metricToggle->setChecked(false);
    metricToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    metricToggle->setArrowType(Qt::RightArrow);
    metricToggle->setText(T("Metric charts", "مخططات المقاييس"));
    metricToggle->setStyleSheet("QToolButton#sectionToggle{font-weight:800;padding:6px 10px;border:none;background:transparent;text-align:left;} QToolButton#sectionToggle:hover{color:#4f86f7;}");
    vl->addWidget(metricToggle);

    auto* metricHost = new QWidget;
    metricHost->setVisible(false);
    m_metricLayout = new QVBoxLayout(metricHost);
    m_metricLayout->setContentsMargins(0, 0, 0, 0);
    m_metricLayout->setSpacing(8);
    vl->addWidget(metricHost);
    connect(metricToggle, &QToolButton::toggled, this, [metricHost, metricToggle](bool on) {
        metricHost->setVisible(on);
        metricToggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
    });

    auto* sec2 = new QLabel(T("Custom comparisons", "المقارنات المخصصة"));
    sec2->setObjectName("section");
    vl->addWidget(sec2);

    auto* compareHost = new QWidget;
    m_compareLayout = new QVBoxLayout(compareHost);
    m_compareLayout->setContentsMargins(0, 0, 0, 0);
    m_compareLayout->setSpacing(8);
    vl->addWidget(compareHost);

    const QList<ChartRequest> previous = data.chartRequests;

    // One row per metric type so the user can always edit the standard charts.
    for (const auto& def : kMetricDefs) {
        ChartKind kind = ChartKind::Candle;
        QList<int> months;
        bool enabled = false;

        for (const auto& req : previous) {
            if (requestIsCompareKind(req.kind))
                continue;
            if (req.metricA != def.id)
                continue;
            kind = req.kind;
            months = req.months;
            enabled = true;
            break;
        }

        if (kind != ChartKind::Pie && kind != ChartKind::Candle && kind != ChartKind::MetricBar && kind != ChartKind::MetricLine)
            kind = ChartKind::Candle;
        appendMetricRow(def.id, kind, months);
        if (!m_metricRows.isEmpty() && m_metricRows.last().enabled)
            m_metricRows.last().enabled->setChecked(enabled);
    }

    bool hasCompareRows = false;
    for (const auto& req : previous) {
        if (requestIsCompareKind(req.kind)) {
            appendCompareRow(&req);
            hasCompareRows = true;
        }
    }
    if (!hasCompareRows)
        appendCompareRow(nullptr);

    auto* addBtn = new QPushButton(T("+  Add Comparison", "+  إضافة مقارنة"));
    addBtn->setObjectName("addBtn");
    addBtn->setCursor(Qt::PointingHandCursor);
    vl->addWidget(addBtn, 0, Qt::AlignLeft);
    connect(addBtn, &QPushButton::clicked, this, [this]() {
        appendCompareRow(nullptr);
    });

    vl->addStretch();
    scroll->setWidget(body);
    root->addWidget(scroll, 1);

    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);
    auto* cancel = new QPushButton(T("Cancel", "إلغاء"));
    cancel->setObjectName("cancelBtn");
    auto* show = new QPushButton(T("▶  Show Results", "▶  عرض النتائج"));
    show->setObjectName("showBtn");
    btnRow->addStretch();
    btnRow->addWidget(cancel);
    btnRow->addWidget(show);
    root->addLayout(btnRow);

    connect(show, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    setMinimumSize(1100, 680);
    resize(1220, 760);
}

void ChartSelectionDialog::appendMetricRow(MetricId id, ChartKind kind, const QList<int>& months, int insertAt)
{
    const MetricDef* def = metricDefForId(id);

    auto* frame = new QFrame;
    frame->setObjectName("row");
    auto* hl = new QHBoxLayout(frame);
    hl->setContentsMargins(14, 10, 14, 10);
    hl->setSpacing(10);

    auto* enabled = new QCheckBox(T("Show", "عرض"));
    enabled->setChecked(false);
    enabled->setToolTip(T("Include this chart in the results", "إظهار هذا المخطط في النتائج"));
    hl->addWidget(enabled);

    auto* name = new QLabel(metricDisplayName(id));
    name->setObjectName("metricName");
    name->setMinimumWidth(170);
    hl->addWidget(name);

    auto* type = new QComboBox;
    type->addItem(T("Grouped bar", "أعمدة مجمعة"), int(ChartKind::MetricBar));
    type->addItem(T("Line", "خط"), int(ChartKind::MetricLine));
    type->addItem(T("Pie", "دائري"), int(ChartKind::Pie));
    type->addItem(T("Candle", "شمعة"), int(ChartKind::Candle));
    if (kind != ChartKind::MetricBar && kind != ChartKind::MetricLine && kind != ChartKind::Pie && kind != ChartKind::Candle)
        kind = ChartKind::Candle;
    int kindIdx = type->findData(int(kind));
    if (kindIdx < 0) kindIdx = 0;
    type->setCurrentIndex(kindIdx);
    type->setMinimumWidth(130);
    if (type->view()) {
        type->view()->setAttribute(Qt::WA_StyledBackground, true);
        type->view()->setStyleSheet(g_lightMode
            ? "QListView{background:#ffffff;color:#1e2340;selection-background-color:#eef0fa;selection-color:#1e2340;border:1px solid #dde2f0;} QListView::item{padding:6px 8px;} QListView::item:selected{background:#eef0fa;color:#1e2340;}"
              : "QListView{background:#1a1f38;color:#c8d0ed;selection-background-color:#4f86f7;selection-color:#ffffff;border:1px solid #252b52;} QListView::item{padding:6px 8px;} QListView::item:selected{background:#4f86f7;color:#ffffff;}");
    }
    hl->addWidget(type);

    auto* monthsBtn = new QToolButton;
    monthsBtn->setObjectName("monthBtn");
    monthsBtn->setPopupMode(QToolButton::InstantPopup);
    monthsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    monthsBtn->setArrowType(Qt::DownArrow);
    monthsBtn->setToolTip(T("Choose months", "اختيار الأشهر"));

    QVector<QAction*> monthActs;
    auto* menu = makeMonthMenu(monthsBtn, months, months.isEmpty(), &monthActs);
    menu->setStyleSheet(g_lightMode
        ? "QMenu{background:#ffffff;color:#1e2340;border:1px solid #dde2f0;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#eef0fa;} QMenu::separator{height:1px;background:#dde2f0;margin:4px 0;}"
        : "QMenu{background:#1a1f38;color:#c8d0ed;border:1px solid #252b52;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#4f86f7;color:#ffffff;} QMenu::separator{height:1px;background:#252b52;margin:4px 0;}");
    monthsBtn->setMenu(menu);
    { QList<int> sel; for (int i = 0; i < 12 && i < monthActs.size(); ++i) if (monthActs[i]->isChecked()) sel << i; updateMonthButtonText(monthsBtn, monthActs); }
    hl->addWidget(monthsBtn);

    auto* duplicateBtn = new QPushButton(T("Duplicate", "تكرار"));
    duplicateBtn->setObjectName("dupBtn");
    duplicateBtn->setCursor(Qt::PointingHandCursor);
    hl->addWidget(duplicateBtn);

    auto* removeBtn = new QPushButton("×");
    removeBtn->setObjectName("removeBtn");
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setFixedSize(28, 28);
    hl->addWidget(removeBtn);


    QObject::connect(enabled, &QCheckBox::toggled, frame, [type, monthsBtn, duplicateBtn, removeBtn](bool on) {
        if (type) type->setEnabled(on);
        if (monthsBtn) monthsBtn->setEnabled(on);
        if (duplicateBtn) duplicateBtn->setEnabled(on);
        if (removeBtn) removeBtn->setEnabled(on);
    });
    type->setEnabled(false);
    monthsBtn->setEnabled(false);
    duplicateBtn->setEnabled(false);
    removeBtn->setEnabled(false);
    enabled->setChecked(false);

    hl->addStretch();

    MetricRow row;
    row.id = id;
    row.frame = frame;
    row.enabled = enabled;
    row.type = type;
    row.monthsBtn = monthsBtn;
    row.monthsMenu = menu;
    row.monthActions = monthActs;
    row.duplicateBtn = duplicateBtn;
    row.removeBtn = removeBtn;

    const int pos = (insertAt < 0 || insertAt > m_metricRows.size()) ? m_metricRows.size() : insertAt;
    m_metricRows.insert(pos, row);
    if (pos >= m_metricLayout->count())
        m_metricLayout->addWidget(frame);
    else
        m_metricLayout->insertWidget(pos, frame);

    syncMonthButton(m_metricRows[pos]);

    connect(duplicateBtn, &QPushButton::clicked, this, [this, frame]() {
        int idx = -1;
        for (int i = 0; i < m_metricRows.size(); ++i) {
            if (m_metricRows[i].frame == frame) { idx = i; break; }
        }
        if (idx < 0) return;
        appendMetricRow(m_metricRows[idx].id,
                        m_metricRows[idx].type ? ChartKind(m_metricRows[idx].type->currentData().toInt()) : ChartKind::Candle,
                        selectedMonths(m_metricRows[idx]),
                        idx + 1);
    });

    connect(removeBtn, &QPushButton::clicked, this, [this, frame]() {
        for (int i = 0; i < m_metricRows.size(); ++i) {
            if (m_metricRows[i].frame == frame) {
                removeMetricRow(i);
                return;
            }
        }
    });
}

void ChartSelectionDialog::removeMetricRow(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= m_metricRows.size()) return;
    auto row = m_metricRows.takeAt(rowIndex);
    if (row.frame) {
        m_metricLayout->removeWidget(row.frame);
        row.frame->deleteLater();
    }
}

void ChartSelectionDialog::syncMonthButton(MetricRow& row)
{
    updateMonthButtonText(row.monthsBtn, row.monthActions);
}

QList<int> ChartSelectionDialog::selectedMonths(const MetricRow& row) const
{
    QList<int> out;
    for (int i = 0; i < row.monthActions.size() && i < 12; ++i) {
        if (row.monthActions[i] && row.monthActions[i]->isChecked())
            out << i;
    }
    if (out.size() == 12)
        out.clear();
    return out;
}


void ChartSelectionDialog::appendCompareRow(const ChartRequest* preset)
{
    const int rowIndex = m_compareRows.size();

    auto* row = new QFrame;
    row->setObjectName("row");
    auto* grid = new QGridLayout(row);
    grid->setContentsMargins(14, 10, 14, 10);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);

    auto makeHdr = [](const QString& txt) {
        auto* l = new QLabel(txt);
        l->setStyleSheet("font-weight:700;color:#8892b8;background:transparent;");
        return l;
    };

    auto* left = new QComboBox;
    auto* right = new QComboBox;
    auto styleComboPopup = [](QComboBox* box) {
        if (!box || !box->view()) return;
        box->view()->setAttribute(Qt::WA_StyledBackground, true);
        box->view()->setStyleSheet(g_lightMode
            ? "QListView{background:#ffffff;color:#1e2340;selection-background-color:#eef0fa;selection-color:#1e2340;border:1px solid #dde2f0;}"
              "QListView::item{padding:6px 8px;}"
              "QListView::item:selected{background:#eef0fa;color:#1e2340;}"
            : "QListView{background:#1a1f38;color:#c8d0ed;selection-background-color:#4f86f7;selection-color:#ffffff;border:1px solid #252b52;}"
              "QListView::item{padding:6px 8px;}"
              "QListView::item:selected{background:#4f86f7;color:#ffffff;}");
    };

    const auto metrics = compareMetrics();
    for (const auto& m : metrics) {
        left->addItem(m.second, int(m.first));
        right->addItem(m.second, int(m.first));
    }
    if (left->count() > 0) left->setCurrentIndex(0);
    if (right->count() > 1) right->setCurrentIndex(1);
    styleComboPopup(left);
    styleComboPopup(right);

    auto* type = new QComboBox;
    type->addItem(T("Grouped bar", "أعمدة مجمعة"), int(ChartKind::CompareBar));
    type->addItem(T("Line", "خط"), int(ChartKind::CompareLine));
    type->addItem(T("Pie", "دائري"), int(ChartKind::ComparePie));
    type->addItem(T("Candle", "شمعة"), int(ChartKind::Candle));
    styleComboPopup(type);

    auto* monthBtn = new QToolButton;
    monthBtn->setObjectName("monthBtn");
    monthBtn->setPopupMode(QToolButton::InstantPopup);
    monthBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    monthBtn->setArrowType(Qt::DownArrow);
    monthBtn->setToolTip(T("Choose months", "اختيار الأشهر"));

    QVector<QAction*> cmpMonthActs;
    QList<int> presetMonths;
    QString presetTitle;
    if (preset) {
        if (preset->metricA >= M_SALES && preset->metricA < M_COUNT)
            left->setCurrentIndex(left->findData(int(preset->metricA)));
        if (preset->metricB >= M_SALES && preset->metricB < M_COUNT)
            right->setCurrentIndex(right->findData(int(preset->metricB)));
        const int typeIndex = type->findData(int(preset->kind));
        if (typeIndex >= 0) type->setCurrentIndex(typeIndex);
        presetMonths = preset->months;
        presetTitle = stripGeneratedMonthSuffix(preset->title);
    }
    auto* monthMenu = makeMonthMenu(monthBtn, presetMonths, presetMonths.isEmpty(), &cmpMonthActs);
    monthMenu->setStyleSheet(g_lightMode
        ? "QMenu{background:#ffffff;color:#1e2340;border:1px solid #dde2f0;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#eef0fa;} QMenu::separator{height:1px;background:#dde2f0;margin:4px 0;}"
        : "QMenu{background:#1a1f38;color:#c8d0ed;border:1px solid #252b52;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#4f86f7;color:#ffffff;} QMenu::separator{height:1px;background:#252b52;margin:4px 0;}");
    monthBtn->setMenu(monthMenu);
    updateCompareMonthButtonText(monthBtn, cmpMonthActs);

    auto* title = new QLineEdit;
    title->setPlaceholderText(T("Comparison title (optional)", "عنوان اختياري"));
    if (!presetTitle.isEmpty())
        title->setText(presetTitle);

    auto* removeBtn = new QPushButton("×");
    removeBtn->setObjectName("removeBtn");
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setFixedSize(28, 28);

    grid->addWidget(makeHdr(T("Left metric", "المقياس الأيسر")),  0, 0);
    grid->addWidget(makeHdr(T("Right metric", "المقياس الأيمن")), 0, 1);
    grid->addWidget(makeHdr(T("Chart type", "نوع الرسم")),  0, 2);
    grid->addWidget(makeHdr(T("Month", "الشهر")),  0, 3);
    grid->addWidget(makeHdr(T("Title", "العنوان")),        0, 4);
    grid->addWidget(left,      1, 0);
    grid->addWidget(right,     1, 1);
    grid->addWidget(type,      1, 2);
    grid->addWidget(monthBtn,   1, 3);
    grid->addWidget(title,     1, 4);
    grid->addWidget(removeBtn, 0, 5, 2, 1, Qt::AlignVCenter);
    grid->setColumnStretch(4, 1);

    m_compareLayout->addWidget(row);

    connect(removeBtn, &QPushButton::clicked, this, [this, row]() {
        for (int i = 0; i < m_compareRows.size(); ++i) {
            if (m_compareRows[i].frame == row) {
                removeCompareRow(i);
                return;
            }
        }
    });

    CompareRow item;
    item.frame = row;
    item.enabled = nullptr;
    item.left = left;
    item.right = right;
    item.type = type;
    item.monthBtn = monthBtn;
    item.monthMenu = monthMenu;
    item.monthActions = cmpMonthActs;
    item.title = title;
    m_compareRows.push_back(item);
}

void ChartSelectionDialog::removeCompareRow(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= m_compareRows.size()) return;
    QFrame* frame = m_compareRows[rowIndex].frame;
    m_compareRows.remove(rowIndex);
    if (frame) {
        m_compareLayout->removeWidget(frame);
        frame->deleteLater();
    }
}

QList<int> ChartSelectionDialog::selectedMonths(const CompareRow& row) const
{
    QList<int> out;
    for (int i = 0; i < row.monthActions.size() && i < 12; ++i) {
        if (row.monthActions[i] && row.monthActions[i]->isChecked())
            out << i;
    }
    if (out.size() == 12)
        out.clear();
    return out;
}

std::array<ChartSel, M_COUNT> ChartSelectionDialog::selections() const
{
    std::array<ChartSel, M_COUNT> out{};
    for (const auto& row : m_metricRows) {
        auto& s = out[row.id];
        const bool on = row.enabled ? row.enabled->isChecked() : false;
        if (!on) {
            s.pie = false;
            s.candle = false;
            continue;
        }
        const ChartKind kind = row.type ? ChartKind(row.type->currentData().toInt()) : ChartKind::Candle;
        s.pie = (kind == ChartKind::Pie);
        s.candle = (kind == ChartKind::Candle || kind == ChartKind::MetricBar || kind == ChartKind::MetricLine);
    }
    return out;
}

QList<ChartRequest> ChartSelectionDialog::chartRequests() const
{
    QList<ChartRequest> out;

    for (const auto& row : m_metricRows) {
        if (!row.enabled || !row.enabled->isChecked())
            continue;

        const QList<int> months = selectedMonths(row);
        const QString monthLabel = monthSummaryText(months);
        const ChartKind kind = row.type ? ChartKind(row.type->currentData().toInt()) : ChartKind::Candle;

        ChartRequest req;
        req.metricA = row.id;
        req.months = months;
        req.kind = kind;
        switch (kind) {
        case ChartKind::Pie:
            req.title = metricDisplayName(row.id) + QStringLiteral(" — ") + T("Pie", "دائري") + QStringLiteral(" (") + monthLabel + QStringLiteral(")");
            break;
        case ChartKind::MetricBar:
            req.title = metricDisplayName(row.id) + QStringLiteral(" — ") + T("Grouped bar", "أعمدة مجمعة") + QStringLiteral(" (") + monthLabel + QStringLiteral(")");
            break;
        case ChartKind::MetricLine:
            req.title = metricDisplayName(row.id) + QStringLiteral(" — ") + T("Line", "خط") + QStringLiteral(" (") + monthLabel + QStringLiteral(")");
            break;
        case ChartKind::Candle:
        default:
            req.title = metricDisplayName(row.id) + QStringLiteral(" — ") + T("Candle", "شمعة") + QStringLiteral(" (") + monthLabel + QStringLiteral(")");
            break;
        }
        out << req;
    }

    for (const auto& row : m_compareRows) {
        if (!row.left || !row.right) continue;
        const auto leftId  = MetricId(row.left->currentData().toInt());
        const auto rightId = MetricId(row.right->currentData().toInt());
        if (leftId == rightId) continue;

        ChartRequest req;
        const int typeVal = row.type ? row.type->currentData().toInt() : int(ChartKind::CompareBar);
        req.kind = static_cast<ChartKind>(typeVal);
        if (req.kind != ChartKind::CompareLine && req.kind != ChartKind::ComparePie && req.kind != ChartKind::Candle)
            req.kind = ChartKind::CompareBar;
        req.metricA = leftId;
        req.metricB = rightId;
        req.months = selectedMonths(row);
        const QString monthLabel = monthSummaryText(req.months);
        req.seriesA = metricDisplayName(leftId);
        req.seriesB = metricDisplayName(rightId);
        req.title = row.title && !row.title->text().trimmed().isEmpty()
            ? row.title->text().trimmed()
            : comparisonTitle(leftId, rightId);
        req.title += QStringLiteral(" (") + monthLabel + QStringLiteral(")");
        out << req;
    }

    return out;
}
