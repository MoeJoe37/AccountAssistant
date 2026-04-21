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
#include <QSizePolicy>
#include <QSignalBlocker>
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
static void rebuildCompareMenuLabels(const QList<MetricId>& selected, QVector<QAction*>& actions)
{
    for (int i = 0; i < actions.size(); ++i) {
        QAction* act = actions[i];
        if (!act) continue;
        const MetricId id = MetricId(act->data().toInt());
        const QString name = metricDisplayName(id);
        const int pos = selected.indexOf(id);
        act->setChecked(pos >= 0);
        act->setText(pos >= 0 ? QString::number(pos + 1) + QStringLiteral(". ") + name : name);
    }
}

static StayOpenMenu* makeMonthMenu(QToolButton* btn, const QList<int>& preChecked,
                                    bool allChecked, QVector<QAction*>* monthActsOut)
{
    auto* menu = new StayOpenMenu(btn);
    const auto names = monthNames();

    // Select All / Deselect All
    auto* selAll  = menu->addAction(tr_select_all_7812c3());
    auto* deselAll= menu->addAction(tr_deselect_all_474bc1());
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
        btn->setText(tr_months_1_b69e08().arg(monthSummaryText(sel)));
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
QPushButton#sectionAddBtn {
    background:#252d4a; color:#c8d0ed;
    border:1px solid #3a4470; border-radius:6px;
    padding:5px 10px; min-height:26px;
    font-weight:700;
}
QPushButton#sectionAddBtn:hover { border-color:#4f86f7; background:#293252; }
QPushButton#metricSelBtn {
    background:#252d4a; color:#c8d0ed;
    border:1px solid #3a4470; border-radius:6px;
    padding:5px 12px; min-height:28px;
    font-weight:700;
}
QPushButton#metricSelBtn:hover {
    border-color:#4f86f7;
    background:#293252;
}
QPushButton#removeBtn {
    background:transparent; color:#5a6490;
    border:none; font-weight:800;
    min-width:28px; max-width:28px;
    font-size:14px;
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
QPushButton#sectionAddBtn {
    background:#ffffff; color:#1e2340;
    border:1px solid #cfd7ea; border-radius:6px;
    padding:5px 10px; min-height:26px;
    font-weight:700;
}
QPushButton#sectionAddBtn:hover { border-color:#4f86f7; background:#eef1fb; }
QPushButton#metricSelBtn {
    background:#ffffff; color:#1e2340;
    border:1px solid #cfd7ea; border-radius:6px;
    padding:5px 12px; min-height:28px;
    font-weight:700;
}
QPushButton#metricSelBtn:hover {
    border-color:#4f86f7;
    background:#eef1fb;
}
QPushButton#removeBtn {
    background:transparent; color:#8892b8;
    border:none; font-weight:800;
    min-width:28px; max-width:28px;
    font-size:14px;
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
        return tr_all_months_428b74();

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
    button->setText(tr_months_1_b69e08().arg(monthSummaryText(selected)));
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
    button->setText(tr_month_1_5fc620().arg(monthSummaryText(selected)));
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

static bool isSupplierMetric(MetricId id)
{
    return id == M_PURCHASES || id == M_SUPPLIER_PAYMENTS;
}

static bool isAccountMetric(MetricId id)
{
    return id == M_EXPENSES || id == M_EXPENSE_AMOUNT || id == M_INVENTORY_OPENING || id == M_INVENTORY_CLOSING;
}

static CompareGroup compareGroupForMetric(MetricId id)
{
    if (isSupplierMetric(id))
        return CompareGroup::Suppliers;
    if (isAccountMetric(id))
        return CompareGroup::Accounts;
    return CompareGroup::General;
}

static CompareGroup compareGroupForMetrics(const QList<MetricId>& metrics)
{
    bool hasSupplier = false;
    bool hasAccount = false;
    for (MetricId id : metrics) {
        const CompareGroup g = compareGroupForMetric(id);
        if (g == CompareGroup::Suppliers)
            hasSupplier = true;
        else if (g == CompareGroup::Accounts)
            hasAccount = true;
    }
    if (hasSupplier)
        return CompareGroup::Suppliers;
    if (hasAccount)
        return CompareGroup::Accounts;
    return CompareGroup::General;
}

static CompareGroup compareGroupForPreset(const ChartRequest* preset)
{
    if (!preset)
        return CompareGroup::General;
    QList<MetricId> metrics = preset->compareMetrics;
    if (metrics.size() < 2) {
        if (preset->metricA >= M_SALES && preset->metricA < M_COUNT) metrics << preset->metricA;
        if (preset->metricB >= M_SALES && preset->metricB < M_COUNT) metrics << preset->metricB;
    }
    return compareGroupForMetrics(metrics);
}

ChartSelectionDialog::ChartSelectionDialog(const AppData& data, QWidget* parent)
    : QDialog(parent)
{
    setStyleSheet(g_lightMode ? kDialogSSLight : kDialogSS);
    setWindowTitle(tr_select_charts_d37b65());
    setMinimumWidth(960);
    setModal(true);
    buildUI(data);
}

void ChartSelectionDialog::buildUI(const AppData& data)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    auto* title = new QLabel(tr_choose_what_appears_in_results_05286a());
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    auto* sub = new QLabel(tr_the_report_shows_each_month_fi_883ac5());
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
    metricToggle->setText(tr_metric_charts_eb2569());
    metricToggle->setStyleSheet("QToolButton#sectionToggle{font-weight:800;padding:6px 10px;border:none;background:transparent;text-align:left;} QToolButton#sectionToggle:hover{color:#4f86f7;}");
    vl->addWidget(metricToggle);

    auto* metricHost = new QWidget;
    metricHost->setVisible(false);
    metricHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* metricHostLayout = new QVBoxLayout(metricHost);
    metricHostLayout->setContentsMargins(0, 0, 0, 0);
    metricHostLayout->setSpacing(8);

    auto* metricControls = new QHBoxLayout;
    metricControls->setContentsMargins(0, 0, 0, 0);
    metricControls->setSpacing(8);
    auto* selectAllBtn = new QPushButton(tr_select_all_48e265());
    selectAllBtn->setObjectName("metricSelBtn");
    auto* deselectAllBtn = new QPushButton(tr_deselect_all_5e3e31());
    deselectAllBtn->setObjectName("metricSelBtn");
    metricControls->addWidget(selectAllBtn);
    metricControls->addWidget(deselectAllBtn);
    metricControls->addStretch();
    metricHostLayout->addLayout(metricControls);

    m_metricLayout = new QVBoxLayout;
    m_metricLayout->setContentsMargins(0, 0, 0, 0);
    m_metricLayout->setSpacing(8);
    metricHostLayout->addLayout(m_metricLayout);

    auto addSectionLabel = [&](const QString& txt) {
        auto* lbl = new QLabel(txt);
        lbl->setObjectName("section");
        m_metricLayout->addWidget(lbl);
        return lbl;
    };

    vl->addWidget(metricHost);
    connect(metricToggle, &QToolButton::toggled, this, [metricHost, metricToggle](bool on) {
        metricHost->setVisible(on);
        metricToggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
    });
    connect(selectAllBtn, &QPushButton::clicked, this, [this]() {
        for (auto& row : m_metricRows)
            if (row.enabled) row.enabled->setChecked(true);
    });
    connect(deselectAllBtn, &QPushButton::clicked, this, [this]() {
        for (auto& row : m_metricRows)
            if (row.enabled) row.enabled->setChecked(false);
    });


    auto* compareHost = new QWidget;
    m_compareLayout = new QVBoxLayout(compareHost);
    m_compareLayout->setContentsMargins(0, 0, 0, 0);
    m_compareLayout->setSpacing(10);

    auto addCompareSection = [&](const QString& txt, QVBoxLayout*& outLayout, CompareGroup group) {
        auto* header = new QHBoxLayout;
        header->setContentsMargins(0, 0, 0, 0);
        header->setSpacing(8);
        auto* lbl = new QLabel(txt);
        lbl->setObjectName("section");
        auto* addBtn = new QPushButton(tr_add_comparison_1c963e());
        addBtn->setObjectName("sectionAddBtn");
        addBtn->setCursor(Qt::PointingHandCursor);
        header->addWidget(lbl);
        header->addStretch();
        header->addWidget(addBtn);
        m_compareLayout->addLayout(header);
        outLayout = new QVBoxLayout;
        outLayout->setContentsMargins(0, 0, 0, 0);
        outLayout->setSpacing(8);
        m_compareLayout->addLayout(outLayout);
        QObject::connect(addBtn, &QPushButton::clicked, this, [this, group]() {
            m_nextCompareGroup = group;
            appendCompareRow(nullptr);
            m_nextCompareGroup = CompareGroup::General;
        });
    };

    addCompareSection(tr_accounts_08f9e5(), m_compareAccountsLayout, CompareGroup::Accounts);
    addCompareSection(tr_suppliers_7beff3(), m_compareSuppliersLayout, CompareGroup::Suppliers);

    auto* generalHeader = new QHBoxLayout;
    generalHeader->setContentsMargins(0, 0, 0, 0);
    generalHeader->setSpacing(8);
    auto* generalLbl = new QLabel(tr_custom_comparisons_63300f());
    generalLbl->setObjectName("section");
    auto* generalAddBtn = new QPushButton(tr_add_comparison_1c963e());
    generalAddBtn->setObjectName("sectionAddBtn");
    generalAddBtn->setCursor(Qt::PointingHandCursor);
    generalHeader->addWidget(generalLbl);
    generalHeader->addStretch();
    generalHeader->addWidget(generalAddBtn);
    m_compareLayout->addLayout(generalHeader);
    m_compareGeneralLayout = new QVBoxLayout;
    m_compareGeneralLayout->setContentsMargins(0, 0, 0, 0);
    m_compareGeneralLayout->setSpacing(8);
    m_compareLayout->addLayout(m_compareGeneralLayout);
    QObject::connect(generalAddBtn, &QPushButton::clicked, this, [this]() {
        m_nextCompareGroup = CompareGroup::General;
        appendCompareRow(nullptr);
    });

    vl->addWidget(compareHost);

    const QList<ChartRequest> previous = data.chartRequests;

    // One row per metric type, grouped by accounts and suppliers.
    addSectionLabel(tr_accounts_08f9e5());
    for (const auto& def : kMetricDefs) {
        if (isSupplierMetric(def.id))
            continue;

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

    addSectionLabel(tr_suppliers_7beff3());
    for (const auto& def : kMetricDefs) {
        if (!isSupplierMetric(def.id))
            continue;

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

    auto* addBtn = new QPushButton(tr_add_comparison_1c963e());
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
    auto* cancel = new QPushButton(tr_cancel_8d40ef());
    cancel->setObjectName("cancelBtn");
    auto* show = new QPushButton(tr_show_results_2b2ce7());
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

    auto* enabled = new QCheckBox(tr_show_9ed617());
    enabled->setChecked(false);
    enabled->setToolTip(tr_include_this_chart_in_the_resu_2a37da());
    hl->addWidget(enabled);

    auto* name = new QLabel(metricDisplayName(id));
    name->setObjectName("metricName");
    name->setMinimumWidth(170);
    hl->addWidget(name);

    auto* type = new QComboBox;
    type->addItem(tr_grouped_bar_82dd84(), int(ChartKind::MetricBar));
    type->addItem(tr_line_133e6e(), int(ChartKind::MetricLine));
    type->addItem(tr_pie_97ce50(), int(ChartKind::Pie));
    type->addItem(tr_candle_77e8b9(), int(ChartKind::Candle));
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
    monthsBtn->setToolTip(tr_choose_months_ff1808());

    QVector<QAction*> monthActs;
    auto* menu = makeMonthMenu(monthsBtn, months, months.isEmpty(), &monthActs);
    menu->setStyleSheet(g_lightMode
        ? "QMenu{background:#ffffff;color:#1e2340;border:1px solid #dde2f0;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#eef0fa;} QMenu::separator{height:1px;background:#dde2f0;margin:4px 0;}"
        : "QMenu{background:#1a1f38;color:#c8d0ed;border:1px solid #252b52;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#4f86f7;color:#ffffff;} QMenu::separator{height:1px;background:#252b52;margin:4px 0;}");
    monthsBtn->setMenu(menu);
    { QList<int> sel; for (int i = 0; i < 12 && i < monthActs.size(); ++i) if (monthActs[i]->isChecked()) sel << i; updateMonthButtonText(monthsBtn, monthActs); }
    hl->addWidget(monthsBtn);

    auto* duplicateBtn = new QPushButton(tr_duplicate_47648b());
    duplicateBtn->setObjectName("dupBtn");
    duplicateBtn->setCursor(Qt::PointingHandCursor);
    hl->addWidget(duplicateBtn);

    auto* removeBtn = new QPushButton(QStringLiteral("X"));
    removeBtn->setObjectName("removeBtn");
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setFixedSize(28, 28);
    removeBtn->setFont(QFont("Segoe UI", 11, QFont::Bold));
    removeBtn->setToolTip(tr_remove_c3a712());
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
    auto* row = new QFrame;
    row->setObjectName("row");
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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

    auto* moreBtn = new QToolButton;
    moreBtn->setObjectName("monthBtn");
    moreBtn->setPopupMode(QToolButton::InstantPopup);
    moreBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    moreBtn->setArrowType(Qt::DownArrow);
    moreBtn->setToolTip(tr_more_metrics_000000());
    auto* moreMenu = new StayOpenMenu(moreBtn);
    moreBtn->setMenu(moreMenu);
    moreMenu->setStyleSheet(g_lightMode
        ? "QMenu{background:#ffffff;color:#1e2340;border:1px solid #dde2f0;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#eef0fa;} QMenu::separator{height:1px;background:#dde2f0;margin:4px 0;}"
        : "QMenu{background:#1a1f38;color:#c8d0ed;border:1px solid #252b52;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#4f86f7;color:#ffffff;} QMenu::separator{height:1px;background:#252b52;margin:4px 0;}");
    QVector<QAction*> moreActs;

    auto* countAs100 = new QComboBox;
    styleComboPopup(countAs100);
    countAs100->setMinimumWidth(170);

    auto* type = new QComboBox;
    type->addItem(tr_grouped_bar_82dd84(), int(ChartKind::CompareBar));
    type->addItem(tr_line_133e6e(), int(ChartKind::CompareLine));
    type->addItem(tr_pie_97ce50(), int(ChartKind::ComparePie));
    type->addItem(tr_candle_77e8b9(), int(ChartKind::Candle));
    styleComboPopup(type);

    auto* monthBtn = new QToolButton;
    monthBtn->setObjectName("monthBtn");
    monthBtn->setPopupMode(QToolButton::InstantPopup);
    monthBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    monthBtn->setArrowType(Qt::DownArrow);
    monthBtn->setToolTip(tr_choose_months_ff1808());

    QVector<QAction*> cmpMonthActs;
    QList<int> presetMonths;
    QString presetTitle;
    QList<MetricId> presetMoreMetrics;
    MetricId presetBase = M_COUNT;
    if (preset) {
        if (preset->compareMetrics.size() >= 2) {
            if (preset->compareMetrics[0] >= M_SALES && preset->compareMetrics[0] < M_COUNT)
                left->setCurrentIndex(left->findData(int(preset->compareMetrics[0])));
            if (preset->compareMetrics[1] >= M_SALES && preset->compareMetrics[1] < M_COUNT)
                right->setCurrentIndex(right->findData(int(preset->compareMetrics[1])));
            for (int i = 2; i < preset->compareMetrics.size(); ++i)
                presetMoreMetrics << preset->compareMetrics[i];
        } else {
            if (preset->metricA >= M_SALES && preset->metricA < M_COUNT)
                left->setCurrentIndex(left->findData(int(preset->metricA)));
            if (preset->metricB >= M_SALES && preset->metricB < M_COUNT)
                right->setCurrentIndex(right->findData(int(preset->metricB)));
        }
        const int typeIndex = type->findData(int(preset->kind));
        if (typeIndex >= 0) type->setCurrentIndex(typeIndex);
        presetMonths = preset->months;
        presetTitle = stripGeneratedMonthSuffix(preset->title);
        if (preset->comparePieBaseMetric >= M_SALES && preset->comparePieBaseMetric < M_COUNT)
            presetBase = preset->comparePieBaseMetric;
    }

    auto* monthMenu = makeMonthMenu(monthBtn, presetMonths, presetMonths.isEmpty(), &cmpMonthActs);
    monthMenu->setStyleSheet(g_lightMode
        ? "QMenu{background:#ffffff;color:#1e2340;border:1px solid #dde2f0;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#eef0fa;} QMenu::separator{height:1px;background:#dde2f0;margin:4px 0;}"
        : "QMenu{background:#1a1f38;color:#c8d0ed;border:1px solid #252b52;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#4f86f7;color:#ffffff;} QMenu::separator{height:1px;background:#252b52;margin:4px 0;}");
    monthBtn->setMenu(monthMenu);
    updateCompareMonthButtonText(monthBtn, cmpMonthActs);

    auto* title = new QLineEdit;
    title->setPlaceholderText(tr_comparison_title_optional_fae13e());
    if (!presetTitle.isEmpty())
        title->setText(presetTitle);

    auto* removeBtn = new QPushButton(QStringLiteral("X"));
    removeBtn->setObjectName("removeBtn");
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setFixedSize(28, 28);
    removeBtn->setFont(QFont("Segoe UI", 11, QFont::Bold));
    removeBtn->setToolTip(tr_remove_c3a712());

    auto* selAll = moreMenu->addAction(tr_select_all_7812c3());
    auto* deselAll = moreMenu->addAction(tr_deselect_all_474bc1());
    moreMenu->addSeparator();
    const auto moreMetricDefs = compareMetrics();
    for (const auto& def : moreMetricDefs) {
        auto* act = moreMenu->addAction(def.second);
        act->setCheckable(true);
        act->setData(int(def.first));
        moreActs << act;
        QObject::connect(act, &QAction::toggled, moreBtn, [this, moreBtn, act](bool checked) {
            const MetricId id = MetricId(act->data().toInt());
            for (auto& r : m_compareRows) {
                if (r.moreBtn != moreBtn) continue;
                if (checked) {
                    if (!r.moreMetrics.contains(id))
                        r.moreMetrics << id;
                } else {
                    r.moreMetrics.removeAll(id);
                }
                syncCompareMoreButton(r);
                syncComparePieBaseControls(r);
                return;
            }
        });
    }
    QObject::connect(selAll, &QAction::triggered, moreBtn, [this, moreBtn]() {
        const auto moreMetricDefs = compareMetrics();
        for (auto& r : m_compareRows) {
            if (r.moreBtn != moreBtn) continue;
            for (const auto& def : moreMetricDefs) {
                if (!r.moreMetrics.contains(def.first))
                    r.moreMetrics << def.first;
            }
            syncCompareMoreButton(r);
            syncComparePieBaseControls(r);
            syncComparePieBaseVisibility();
            return;
        }
    });
    QObject::connect(deselAll, &QAction::triggered, moreBtn, [this, moreBtn]() {
        for (auto& r : m_compareRows) {
            if (r.moreBtn != moreBtn) continue;
            r.moreMetrics.clear();
            syncCompareMoreButton(r);
            syncComparePieBaseControls(r);
            return;
        }
    });

    grid->addWidget(makeHdr(tr_left_metric_c474b3()),  0, 0);
    grid->addWidget(makeHdr(tr_right_metric_1d236d()), 0, 1);
    grid->addWidget(makeHdr(tr_more_metrics_000000()), 0, 2);
    grid->addWidget(makeHdr(tr_chart_type_bd42b2()),    0, 3);
    m_comparePieBaseHdr = makeHdr(tr_count_as_100_percent_4b3a11());
    grid->addWidget(m_comparePieBaseHdr, 0, 4);
    grid->addWidget(makeHdr(tr_month_460756()),         0, 5);
    grid->addWidget(makeHdr(tr_title_c1c427()),         0, 6);
    grid->addWidget(left,         1, 0);
    grid->addWidget(right,        1, 1);
    grid->addWidget(moreBtn,      1, 2);
    grid->addWidget(type,         1, 3);
    grid->addWidget(countAs100,   1, 4);
    grid->addWidget(monthBtn,     1, 5);
    grid->addWidget(title,        1, 6);
    grid->addWidget(removeBtn,    0, 7, 2, 1, Qt::AlignVCenter);
    grid->setColumnStretch(6, 1);
    grid->setColumnStretch(7, 0);

    QVBoxLayout* targetLayout = m_compareGeneralLayout ? m_compareGeneralLayout : m_compareLayout;
    const CompareGroup forcedGroup = m_nextCompareGroup;
    m_nextCompareGroup = CompareGroup::General;
    if (preset) {
        const CompareGroup group = compareGroupForPreset(preset);
        if (group == CompareGroup::Accounts && m_compareAccountsLayout)
            targetLayout = m_compareAccountsLayout;
        else if (group == CompareGroup::Suppliers && m_compareSuppliersLayout)
            targetLayout = m_compareSuppliersLayout;
    } else if (forcedGroup == CompareGroup::Accounts && m_compareAccountsLayout) {
        targetLayout = m_compareAccountsLayout;
    } else if (forcedGroup == CompareGroup::Suppliers && m_compareSuppliersLayout) {
        targetLayout = m_compareSuppliersLayout;
    }
    if (targetLayout)
        targetLayout->addWidget(row);

    CompareRow item;
    item.frame = row;
    item.layout = targetLayout;
    item.left = left;
    item.right = right;
    item.moreBtn = moreBtn;
    item.moreMenu = moreMenu;
    item.moreActions = moreActs;
    item.moreMetrics = presetMoreMetrics;
    item.countAs100 = countAs100;
    item.comparePieBase = presetBase;
    item.type = type;
    item.monthBtn = monthBtn;
    item.monthMenu = monthMenu;
    item.monthActions = cmpMonthActs;
    item.title = title;
    m_compareRows.push_back(item);

    syncCompareMoreButton(m_compareRows.last());
    syncComparePieBaseControls(m_compareRows.last());
    syncComparePieBaseVisibility();

    connect(left, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, row]() {
        for (auto& r : m_compareRows) {
            if (r.frame != row) continue;
            syncComparePieBaseControls(r);
            syncComparePieBaseVisibility();
            return;
        }
    });
    connect(right, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, row]() {
        for (auto& r : m_compareRows) {
            if (r.frame != row) continue;
            syncComparePieBaseControls(r);
            syncComparePieBaseVisibility();
            return;
        }
    });
    connect(countAs100, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, row](int) {
        for (auto& r : m_compareRows) {
            if (r.frame != row || !r.countAs100) continue;
            const int idx = r.countAs100->currentIndex();
            r.comparePieBase = idx >= 0 ? MetricId(r.countAs100->currentData().toInt()) : M_COUNT;
            return;
        }
    });
    connect(type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, row]() {
        for (auto& r : m_compareRows) {
            if (r.frame != row) continue;
            syncComparePieBaseControls(r);
            syncComparePieBaseVisibility();
            return;
        }
    });

    syncComparePieBaseControls(m_compareRows.last());
    connect(removeBtn, &QPushButton::clicked, this, [this, row]() {
        for (int i = 0; i < m_compareRows.size(); ++i) {
            if (m_compareRows[i].frame == row) {
                removeCompareRow(i);
                return;
            }
        }
    });
}
void ChartSelectionDialog::removeCompareRow(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= m_compareRows.size()) return;
    auto row = m_compareRows.takeAt(rowIndex);
    if (row.frame) {
        if (row.layout)
            row.layout->removeWidget(row.frame);
        else if (m_compareLayout)
            m_compareLayout->removeWidget(row.frame);
        row.frame->deleteLater();
    }
    syncComparePieBaseVisibility();
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

QList<MetricId> ChartSelectionDialog::selectedCompareMetrics(const CompareRow& row) const
{
    QList<MetricId> out;
    auto appendUnique = [&](MetricId id) {
        if (id < M_SALES || id >= M_COUNT) return;
        if (!out.contains(id)) out << id;
    };
    if (row.left) appendUnique(MetricId(row.left->currentData().toInt()));
    if (row.right) appendUnique(MetricId(row.right->currentData().toInt()));
    for (MetricId id : row.moreMetrics) appendUnique(id);
    return out;
}

void ChartSelectionDialog::syncCompareMoreButton(CompareRow& row)
{
    if (!row.moreBtn) return;
    row.moreBtn->setText(row.moreMetrics.isEmpty()
        ? QStringLiteral("+  ") + tr_more_metrics_000000()
        : QStringLiteral("+  ") + tr_more_metrics_000000() + QStringLiteral(" (") + QString::number(row.moreMetrics.size()) + QStringLiteral(")"));
    const auto defs = compareMetrics();
    for (int i = 0; i < row.moreActions.size() && i < defs.size(); ++i) {
        QAction* act = row.moreActions[i];
        if (!act) continue;
        const MetricId id = defs[i].first;
        const QString name = defs[i].second;
        const int pos = row.moreMetrics.indexOf(id);
        act->setChecked(pos >= 0);
        act->setText(pos >= 0 ? QString::number(pos + 1) + QStringLiteral(". ") + name : name);
    }
}

void ChartSelectionDialog::syncComparePieBaseControls(CompareRow& row)
{
    const bool show = row.type && ChartKind(row.type->currentData().toInt()) == ChartKind::ComparePie;
    if (row.countAs100) row.countAs100->setVisible(show);
    if (!show || !row.countAs100)
        return;

    const QList<MetricId> metrics = selectedCompareMetrics(row);
    const MetricId previous = row.comparePieBase;
    const bool blocked = row.countAs100->blockSignals(true);
    row.countAs100->clear();
    row.countAs100->addItem(tr_total_a52764(), int(M_COUNT));
    for (MetricId id : metrics) {
        row.countAs100->addItem(metricDisplayName(id), int(id));
    }
    int idx = row.countAs100->findData(int(previous));
    if (idx < 0)
        idx = 0;
    if (idx >= 0)
        row.countAs100->setCurrentIndex(idx);
    row.countAs100->setEnabled(!metrics.isEmpty());
    row.comparePieBase = (idx >= 0) ? MetricId(row.countAs100->currentData().toInt()) : M_COUNT;
    row.countAs100->blockSignals(blocked);
}

void ChartSelectionDialog::syncComparePieBaseVisibility()
{
    const bool show = std::any_of(m_compareRows.cbegin(), m_compareRows.cend(), [](const CompareRow& r) {
        return r.type && ChartKind(r.type->currentData().toInt()) == ChartKind::ComparePie;
    });
    if (m_comparePieBaseHdr)
        m_comparePieBaseHdr->setVisible(show);
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
        s.candle = (kind == ChartKind::Candle);
        s.bar = (kind == ChartKind::MetricBar);
        s.line = (kind == ChartKind::MetricLine);
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
            req.title = metricDisplayName(row.id) + QStringLiteral(" — ") + tr_pie_97ce50() + QStringLiteral(" (") + monthLabel + QStringLiteral(")");
            break;
        case ChartKind::MetricBar:
            req.title = metricDisplayName(row.id) + QStringLiteral(" — ") + tr_grouped_bar_82dd84() + QStringLiteral(" (") + monthLabel + QStringLiteral(")");
            break;
        case ChartKind::MetricLine:
            req.title = metricDisplayName(row.id) + QStringLiteral(" — ") + tr_line_133e6e() + QStringLiteral(" (") + monthLabel + QStringLiteral(")");
            break;
        case ChartKind::Candle:
        default:
            req.title = metricDisplayName(row.id) + QStringLiteral(" — ") + tr_candle_77e8b9() + QStringLiteral(" (") + monthLabel + QStringLiteral(")");
            break;
        }
        out << req;
    }

    for (const auto& row : m_compareRows) {
        const QList<MetricId> metrics = selectedCompareMetrics(row);
        if (metrics.size() < 2) continue;
        if (metrics.first() == metrics.value(1) && metrics.size() == 2) continue;

        ChartRequest req;
        const int typeVal = row.type ? row.type->currentData().toInt() : int(ChartKind::CompareBar);
        req.kind = static_cast<ChartKind>(typeVal);
        if (req.kind != ChartKind::CompareLine && req.kind != ChartKind::ComparePie && req.kind != ChartKind::Candle)
            req.kind = ChartKind::CompareBar;
        req.metricA = metrics.value(0);
        req.metricB = metrics.value(1);
        req.compareMetrics = metrics;
        req.comparePieBaseMetric = row.comparePieBase;
        req.months = selectedMonths(row);
        const QString monthLabel = monthSummaryText(req.months);
        req.seriesA = metricDisplayName(req.metricA);
        req.seriesB = metricDisplayName(req.metricB);
        req.title = row.title && !row.title->text().trimmed().isEmpty()
            ? row.title->text().trimmed()
            : (metrics.size() > 2 ? comparisonTitle(metrics) : comparisonTitle(req.metricA, req.metricB));
        req.title += QStringLiteral(" (") + monthLabel + QStringLiteral(")");
        out << req;
    }

    return out;
}
