#include "Supplierswidget.h"
#include "translations.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QAbstractSpinBox>
#include <QAbstractButton>
#include <QLocale>
#include <QWheelEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QEasingCurve>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QCheckBox>
#include <QComboBox>
#include <QAbstractItemView>

static const char* kRootDark = "QWidget#suppliersRoot{background:#0d1020;} QLabel{background:transparent;}";
static const char* kRootLight = "QWidget#suppliersRoot{background:#f4f6fb;} QLabel{background:transparent;}";
static const char* kCardDark = "QFrame#supplierCard{background:#141827;border:1px solid #252b4a;border-radius:10px;}";
static const char* kCardLight = "QFrame#supplierCard{background:#ffffff;border:1px solid #dde2f0;border-radius:10px;}";
static const char* kHdrDark = "QWidget#supplierHeader{background:#1a1f38;border-radius:9px 9px 0 0;}QWidget#supplierHeader:hover{background:#1e2445;}";
static const char* kHdrLight = "QWidget#supplierHeader{background:#f4f6fb;border-radius:9px 9px 0 0;}QWidget#supplierHeader:hover{background:#eef0fa;}";
static const char* kContentDark = "QWidget#supplierContent{background:#141827;border-radius:0 0 9px 9px;}";
static const char* kContentLight = "QWidget#supplierContent{background:#ffffff;border-radius:0 0 9px 9px;}";
static const char* kMonthLblDark = "color:#c8d0ed;font-weight:800;background:transparent;";
static const char* kMonthLblLight = "color:#1e2340;font-weight:800;background:transparent;";
static const char* kChevronDark = "color:#8892b8;background:transparent;font-weight:800;";
static const char* kChevronLight = "color:#6b7280;background:transparent;font-weight:800;";

class SupplierStayOpenMenu : public QMenu {
public:
    using QMenu::QMenu;
protected:
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        QAction* action = activeAction();
        if (action && action->isEnabled() && action->isCheckable()) {
            action->trigger();
            return;
        }
        QMenu::mouseReleaseEvent(event);
    }
};

static QList<MetricId> supplierGraphMetrics()
{
    return {
        M_PURCHASES,
        M_SUPPLIER_PAYMENTS,
        M_SUPPLIER_PREVIOUS_BALANCE,
        M_SUPPLIER_TOTAL_DEBT,
        M_SUPPLIER_PAYMENT_PCT_PURCHASES,
        M_SUPPLIER_PAYMENT_PCT_DEBT,
        M_SUPPLIER_BALANCE
    };
}

static QList<int> supplierMonthsWithData(const AppData& data)
{
    QList<int> out;
    for (int month = 0; month < 12; ++month) {
        bool hasData = false;
        for (const SupplierEntry& entry : data.supplierEntries[month]) {
            if (!entry.name.trimmed().isEmpty() || entry.previousBalance != 0.0 || entry.purchases != 0.0 ||
                entry.totalDebt != 0.0 || entry.payments != 0.0) {
                hasData = true;
                break;
            }
        }
        if (!hasData) {
            const SupplierMonthData& legacy = data.suppliers[month];
            hasData = !legacy.supplierName.trimmed().isEmpty() || legacy.purchases != 0.0 || legacy.payments != 0.0;
        }
        if (hasData)
            out << month;
    }
    return out;
}

static QString supplierMonthSummaryText(const QList<int>& months)
{
    if (months.isEmpty() || months.size() == 12)
        return tr_all_months_428b74();
    const QStringList names = monthNames();
    QStringList selected;
    for (int month : months) {
        if (month >= 0 && month < names.size())
            selected << names.value(month);
    }
    return selected.isEmpty() ? tr_all_months_428b74() : selected.join(QStringLiteral(", "));
}

class SupplierGraphSelectionDialog : public QDialog {
public:
    SupplierGraphSelectionDialog(ChartKind kind, const AppData& data, QWidget* parent = nullptr, const ChartRequest* existing = nullptr)
        : QDialog(parent), m_kind(kind)
    {
        setWindowTitle(tr_show_graphs_26cf20());
        setModal(true);
        setMinimumWidth(520);
        setStyleSheet(g_lightMode
            ? QStringLiteral("QDialog{background:#f4f6fb;} QLabel{color:#1e2340;background:transparent;} QToolButton,QPushButton,QComboBox{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:7px;padding:7px 12px;font-weight:700;} QToolButton:hover,QPushButton:hover,QComboBox:hover{background:#eef0fa;} QMenu{background:#ffffff;color:#1e2340;border:1px solid #dde2f0;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#eef0fa;} QCheckBox{color:#1e2340;background:transparent;font-weight:700;} QCheckBox::indicator{width:17px;height:17px;border:1px solid #8fa1c2;border-radius:4px;background:#ffffff;} QCheckBox::indicator:checked{background:#4f86f7;border:1px solid #356ed6;} QCheckBox::indicator:disabled{background:#eef0fa;border:1px solid #cfd7ea;}")
            : QStringLiteral("QDialog{background:#12152a;} QLabel{color:#e6ebff;background:transparent;} QToolButton,QPushButton,QComboBox{background:#1a1f38;color:#e7ecff;border:1px solid #343c63;border-radius:7px;padding:7px 12px;font-weight:700;} QToolButton:hover,QPushButton:hover,QComboBox:hover{background:#1e2445;} QMenu{background:#1a1f38;color:#e7ecff;border:1px solid #343c63;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#4f86f7;color:#ffffff;} QCheckBox{color:#e6ebff;background:transparent;font-weight:700;} QCheckBox::indicator{width:17px;height:17px;border:1px solid #59648c;border-radius:4px;background:#12152a;} QCheckBox::indicator:checked{background:#4f86f7;border:1px solid #7ba7ff;} QCheckBox::indicator:disabled{background:#1a1f38;border:1px solid #343c63;}")
        );

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(18, 16, 18, 16);
        root->setSpacing(12);

        auto styleComboPopup = [](QComboBox* box) {
            if (!box || !box->view()) return;
            box->view()->setAttribute(Qt::WA_StyledBackground, true);
            box->view()->setStyleSheet(g_lightMode
                ? QStringLiteral("QListView{background:#ffffff;color:#1e2340;selection-background-color:#eef0fa;selection-color:#1e2340;border:1px solid #dde2f0;} QListView::item{padding:6px 8px;} QListView::item:selected{background:#eef0fa;color:#1e2340;}")
                : QStringLiteral("QListView{background:#1a1f38;color:#c8d0ed;selection-background-color:#4f86f7;selection-color:#ffffff;border:1px solid #252b52;} QListView::item{padding:6px 8px;} QListView::item:selected{background:#4f86f7;color:#ffffff;}")
            );
        };

        auto* chartTypeLabel = new QLabel(tr_chart_type_bd42b2());
        chartTypeLabel->setStyleSheet(QStringLiteral("font-weight:800;"));
        root->addWidget(chartTypeLabel);

        m_chartTypeCombo = new QComboBox(this);
        styleComboPopup(m_chartTypeCombo);
        m_chartTypeCombo->setMinimumWidth(220);
        m_chartTypeCombo->addItem(tr_pie_chart_9d4e04(), int(ChartKind::ComparePie));
        m_chartTypeCombo->addItem(tr_bar_chart_a5f324(), int(ChartKind::CompareBar));
        m_chartTypeCombo->addItem(T("Horizontal bar", "شريط أفقي"), int(ChartKind::HorizontalBar));
        m_chartTypeCombo->addItem(tr_line_chart_932796(), int(ChartKind::CompareLine));
        m_chartTypeCombo->addItem(tr_candle_chart_f7a9c2(), int(ChartKind::Candle));
        int typeIndex = m_chartTypeCombo->findData(int(m_kind));
        if (typeIndex < 0)
            typeIndex = m_chartTypeCombo->findData(int(ChartKind::CompareBar));
        if (typeIndex < 0)
            typeIndex = 0;
        m_chartTypeCombo->setCurrentIndex(typeIndex);
        m_kind = static_cast<ChartKind>(m_chartTypeCombo->currentData().toInt());
        connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            if (!m_chartTypeCombo) return;
            m_kind = static_cast<ChartKind>(m_chartTypeCombo->currentData().toInt());
            updateSummaryAvailability();
        });
        root->addWidget(m_chartTypeCombo);

        auto* title = new QLabel(tr_auto_graph_metrics_b363616c());
        title->setObjectName("section");
        title->setStyleSheet(QStringLiteral("font-weight:800;font-size:15px;"));
        root->addWidget(title);

        m_metricsBtn = new QToolButton(this);
        m_metricsBtn->setPopupMode(QToolButton::InstantPopup);
        m_metricsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_metricsBtn->setArrowType(Qt::DownArrow);
        auto* metricsMenu = new SupplierStayOpenMenu(m_metricsBtn);
        QAction* selectAll = metricsMenu->addAction(tr_select_all_7812c3());
        QAction* deselectAll = metricsMenu->addAction(tr_deselect_all_474bc1());
        metricsMenu->addSeparator();
        for (MetricId id : supplierGraphMetrics()) {
            QAction* action = metricsMenu->addAction(metricDisplayName(id));
            action->setCheckable(true);
            action->setData(int(id));
            action->setChecked(existing ? ((!existing->compareMetrics.isEmpty() && existing->compareMetrics.contains(id)) || (existing->compareMetrics.isEmpty() && id == existing->metricA)) : id == M_SUPPLIER_BALANCE);
            m_metricActions << action;
            connect(action, &QAction::toggled, this, [this]() { updateMetricButton(); });
        }
        connect(selectAll, &QAction::triggered, this, [this]() {
            for (QAction* action : m_metricActions)
                if (action) action->setChecked(true);
            updateMetricButton();
        });
        connect(deselectAll, &QAction::triggered, this, [this]() {
            for (QAction* action : m_metricActions)
                if (action) action->setChecked(false);
            updateMetricButton();
        });
        m_metricsBtn->setMenu(metricsMenu);
        root->addWidget(m_metricsBtn);

        auto* countAsLabel = new QLabel(tr_count_as_100_percent_4b3a11());
        countAsLabel->setStyleSheet(QStringLiteral("font-weight:800;"));
        root->addWidget(countAsLabel);

        m_countAs100 = new QComboBox(this);
        styleComboPopup(m_countAs100);
        m_countAs100->setMinimumWidth(220);
        root->addWidget(m_countAs100);

        auto* monthsLabel = new QLabel(tr_choose_months_ff1808());
        monthsLabel->setStyleSheet(QStringLiteral("font-weight:800;"));
        root->addWidget(monthsLabel);

        m_monthsBtn = new QToolButton(this);
        m_monthsBtn->setPopupMode(QToolButton::InstantPopup);
        m_monthsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_monthsBtn->setArrowType(Qt::DownArrow);
        auto* monthsMenu = new SupplierStayOpenMenu(m_monthsBtn);
        QAction* selectAllMonths = monthsMenu->addAction(tr_select_all_7812c3());
        QAction* deselectAllMonths = monthsMenu->addAction(tr_deselect_all_474bc1());
        monthsMenu->addSeparator();
        const QList<int> dataMonths = supplierMonthsWithData(data);
        const bool useDataMonths = !dataMonths.isEmpty();
        const QStringList months = monthNames();
        for (int i = 0; i < 12; ++i) {
            QAction* action = monthsMenu->addAction(months.value(i));
            action->setCheckable(true);
            action->setData(i);
            action->setChecked(existing ? (existing->months.isEmpty() || existing->months.contains(i)) : (useDataMonths ? dataMonths.contains(i) : true));
            m_monthActions << action;
            connect(action, &QAction::toggled, this, [this]() { updateMonthButton(); });
        }
        connect(selectAllMonths, &QAction::triggered, this, [this]() {
            for (QAction* action : m_monthActions)
                if (action) action->setChecked(true);
            updateMonthButton();
        });
        connect(deselectAllMonths, &QAction::triggered, this, [this]() {
            for (QAction* action : m_monthActions)
                if (action) action->setChecked(false);
            updateMonthButton();
        });
        m_monthsBtn->setMenu(monthsMenu);
        root->addWidget(m_monthsBtn);

        m_summaryCheck = new QCheckBox(tr_auto_summary_7c91cb2b(), this);
        m_summaryCheck->setToolTip(tr_auto_add_a_summary_total_at_the_end_of_the_grap_fc422aba());
        if (existing)
            m_summaryCheck->setChecked(existing->includeSummaryPoint);
        updateSummaryAvailability();
        root->addWidget(m_summaryCheck);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
        if (QPushButton* ok = buttons->button(QDialogButtonBox::Ok))
            ok->setText(tr_show_graphs_26cf20());
        if (QPushButton* cancel = buttons->button(QDialogButtonBox::Cancel))
            cancel->setText(tr_cancel_8d40ef());
        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            if (selectedMetrics().isEmpty()) {
                QMessageBox::warning(this, tr_show_graphs_26cf20(), tr_no_charts_selected_7a4c8f());
                return;
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        root->addWidget(buttons);

        updateMetricButton();
        if (existing && m_countAs100) {
            int idx = m_countAs100->findData(int(existing->comparePieBaseMetric));
            if (idx < 0) idx = 0;
            m_countAs100->setCurrentIndex(idx);
        }
        updateMonthButton();
    }

    ChartKind kind() const { return m_kind; }

    QList<MetricId> selectedMetrics() const
    {
        QList<MetricId> out;
        for (QAction* action : m_metricActions) {
            if (action && action->isChecked())
                out << MetricId(action->data().toInt());
        }
        return out;
    }

    QList<int> selectedMonths() const
    {
        QList<int> out;
        for (QAction* action : m_monthActions) {
            if (action && action->isChecked())
                out << action->data().toInt();
        }
        if (out.isEmpty() || out.size() == 12)
            return {};
        return out;
    }

    bool includeSummaryPoint() const
    {
        return m_summaryCheck && m_summaryCheck->isChecked() && m_kind != ChartKind::Pie && m_kind != ChartKind::ComparePie;
    }

    MetricId countAs100Metric() const
    {
        if (!m_countAs100)
            return M_COUNT;
        const int idx = m_countAs100->currentIndex();
        if (idx < 0)
            return M_COUNT;
        return MetricId(m_countAs100->currentData().toInt());
    }

private:
    void updateMetricButton()
    {
        const int count = selectedMetrics().size();
        m_metricsBtn->setText(count <= 0
            ? QStringLiteral("+  ") + tr_auto_graph_metrics_b363616c()
            : QStringLiteral("+  ") + tr_auto_graph_metrics_b363616c() + QStringLiteral(" (") + QString::number(count) + QStringLiteral(")"));
        syncCountAs100Options();
    }

    void updateSummaryAvailability()
    {
        if (!m_summaryCheck)
            return;
        const bool isPie = (m_kind == ChartKind::Pie || m_kind == ChartKind::ComparePie);
        m_summaryCheck->setEnabled(!isPie);
        if (isPie)
            m_summaryCheck->setChecked(false);
    }

    void syncCountAs100Options()
    {
        if (!m_countAs100)
            return;
        const MetricId previous = countAs100Metric();
        const bool blocked = m_countAs100->blockSignals(true);
        m_countAs100->clear();
        m_countAs100->addItem(tr_total_a52764(), int(M_COUNT));
        const QList<MetricId> metrics = selectedMetrics();
        for (MetricId id : metrics)
            m_countAs100->addItem(metricDisplayName(id), int(id));
        int idx = m_countAs100->findData(int(previous));
        if (idx < 0)
            idx = 0;
        m_countAs100->setCurrentIndex(idx);
        m_countAs100->setEnabled(!metrics.isEmpty());
        m_countAs100->blockSignals(blocked);
    }

    void updateMonthButton()
    {
        QList<int> months;
        for (QAction* action : m_monthActions) {
            if (action && action->isChecked())
                months << action->data().toInt();
        }
        m_monthsBtn->setText(tr_months_1_b69e08().arg(supplierMonthSummaryText(months)));
    }

    ChartKind m_kind;
    QToolButton* m_metricsBtn{};
    QToolButton* m_monthsBtn{};
    QComboBox* m_chartTypeCombo{};
    QComboBox* m_countAs100{};
    QCheckBox* m_summaryCheck{};
    QVector<QAction*> m_metricActions;
    QVector<QAction*> m_monthActions;
};


SupplierSpinBox::SupplierSpinBox(QWidget* parent) : QDoubleSpinBox(parent)
{
    setRange(0, 1e12);
    setDecimals(currencyDecimals());
    setSingleStep(100);
    setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    setGroupSeparatorShown(true);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    updatePrefix();
}
void SupplierSpinBox::updatePrefix() { setPrefix(currencyPrefix()); setSuffix(currencySuffix()); setDecimals(currencyDecimals()); }
void SupplierSpinBox::wheelEvent(QWheelEvent* e) { e->ignore(); }
void SupplierSpinBox::focusInEvent(QFocusEvent* e) { QDoubleSpinBox::focusInEvent(e); selectAll(); }

SupplierMonthCard::SupplierMonthCard(int monthIndex, QWidget* parent) : QFrame(parent), m_monthIndex(monthIndex)
{
    setObjectName("supplierCard");
    setAttribute(Qt::WA_StyledBackground, true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_header = new QWidget;
    m_header->setObjectName("supplierHeader");
    m_header->setFixedHeight(52);
    m_header->setCursor(Qt::PointingHandCursor);
    m_header->setAttribute(Qt::WA_StyledBackground, true);
    auto* top = new QHBoxLayout(m_header);
    top->setContentsMargins(16, 0, 16, 0);
    top->setSpacing(10);
    m_monthLabel = new QLabel;
    m_chevron = new QLabel("▼");
    m_addBtn = new QPushButton;
    m_addBtn->setCursor(Qt::PointingHandCursor);
    m_addBtn->setFixedHeight(30);
    top->addWidget(m_monthLabel);
    top->addStretch();
    top->addWidget(m_addBtn);
    top->addWidget(m_chevron);
    m_header->installEventFilter(this);
    root->addWidget(m_header);

    m_content = new QWidget;
    m_content->setObjectName("supplierContent");
    m_content->setAttribute(Qt::WA_StyledBackground, true);
    auto* contentLayout = new QVBoxLayout(m_content);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(8);

    m_labelsRow = new QWidget;
    m_labelsLayout = new QGridLayout(m_labelsRow);
    m_labelsLayout->setContentsMargins(0,0,0,0);
    m_labelsLayout->setHorizontalSpacing(6);
    m_labelsLayout->setVerticalSpacing(2);
    m_labelsRow->setVisible(false);

    m_rowsLayout = new QVBoxLayout;
    m_rowsLayout->setContentsMargins(0,0,0,0);
    m_rowsLayout->setSpacing(6);
    contentLayout->addLayout(m_rowsLayout);

    root->addWidget(m_content);

    m_anim = new QPropertyAnimation(this, "contentHeight", this);
    m_anim->setDuration(220);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_addBtn, &QPushButton::clicked, this, &SupplierMonthCard::addSupplierRequested);
    setRowCount(1);

    if (m_monthIndex == 0) {
        m_content->setVisible(true);
        m_expanded = true;
        m_chevron->setText("▲");
        QTimer::singleShot(0, this, [this]{ m_fullHeight = qMax(10, m_content->sizeHint().height()); m_content->setMaximumHeight(m_fullHeight); });
    } else {
        m_content->setVisible(false);
        m_content->setMaximumHeight(0);
    }

    retranslate();
    applyTheme();
}

SupplierSpinBox* SupplierMonthCard::makeSpin(bool readOnly)
{
    auto* s = new SupplierSpinBox;
    s->setReadOnly(readOnly);
    s->setButtonSymbols(QAbstractSpinBox::NoButtons);
    s->setFocusPolicy(readOnly ? Qt::NoFocus : Qt::StrongFocus);
    if (readOnly)
        s->setSpecialValueText(QString());
    return s;
}

QLabel* SupplierMonthCard::makeResultLabel()
{
    auto* l = new QLabel;
    l->setAlignment(Qt::AlignCenter);
    l->setMinimumWidth(145);
    l->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    l->setFixedHeight(32);
    return l;
}

void SupplierMonthCard::appendRow()
{
    RowWidgets rw;
    rw.row = new QWidget;
    auto* grid = new QGridLayout(rw.row);
    grid->setContentsMargins(0,0,0,0);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(3);

    auto mkFieldLabel = [this](const QString& text) {
        auto* lbl = new QLabel(text);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setWordWrap(false);
        lbl->setProperty("supplierFieldLabel", true);
        lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        lbl->setFixedHeight(24);
        return lbl;
    };

    rw.nameLabel = mkFieldLabel(tr_auto_supplier_name_ac45e726());
    rw.previousBalanceLabel = mkFieldLabel(tr_auto_previous_balance_d6da85a6());
    rw.purchasesLabel = mkFieldLabel(tr_auto_purchases_eb5647b3());
    rw.totalDebtLabel = mkFieldLabel(tr_auto_total_debt_b9772183());
    rw.paymentsLabel = mkFieldLabel(tr_auto_payments_726d1e53());
    rw.pctPurchasesLabel = mkFieldLabel(tr_auto_payment_of_purchases_81a9c0e3());
    rw.pctDebtLabel = mkFieldLabel(tr_auto_payment_of_debt_ba7e4d60());
    rw.balanceLabel = mkFieldLabel(tr_auto_supplier_balance_74852681());

    rw.name = new QLineEdit;
    rw.name->setMinimumWidth(180);
    rw.name->setFixedHeight(32);
    rw.name->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rw.previousBalance = makeSpin(m_monthIndex != 0);
    rw.previousBalance->setMinimumWidth(135);
    rw.previousBalance->setFixedHeight(32);
    rw.purchases = makeSpin(false);
    rw.purchases->setMinimumWidth(135);
    rw.purchases->setFixedHeight(32);
    rw.totalDebt = makeSpin(true);
    rw.totalDebt->setMinimumWidth(135);
    rw.totalDebt->setFixedHeight(32);
    rw.payments = makeSpin(false);
    rw.payments->setMinimumWidth(135);
    rw.payments->setFixedHeight(32);
    rw.pctPurchases = makeResultLabel();
    rw.pctDebt = makeResultLabel();
    rw.balance = makeResultLabel();

    const int lr = 0;
    const int wr = 1;
    grid->addWidget(rw.nameLabel, lr, 0);
    grid->addWidget(rw.previousBalanceLabel, lr, 1);
    grid->addWidget(rw.purchasesLabel, lr, 2);
    grid->addWidget(rw.totalDebtLabel, lr, 3);
    grid->addWidget(rw.paymentsLabel, lr, 4);
    grid->addWidget(rw.pctPurchasesLabel, lr, 5);
    grid->addWidget(rw.pctDebtLabel, lr, 6);
    grid->addWidget(rw.balanceLabel, lr, 7);
    grid->addWidget(rw.name, wr, 0);
    grid->addWidget(rw.previousBalance, wr, 1);
    grid->addWidget(rw.purchases, wr, 2);
    grid->addWidget(rw.totalDebt, wr, 3);
    grid->addWidget(rw.payments, wr, 4);
    grid->addWidget(rw.pctPurchases, wr, 5);
    grid->addWidget(rw.pctDebt, wr, 6);
    grid->addWidget(rw.balance, wr, 7);
    for (int c = 0; c < 8; ++c)
        grid->setColumnStretch(c, c == 0 ? 2 : 1);

    const int idx = m_rows.size();
    connect(rw.name, &QLineEdit::textChanged, this, [this, idx](const QString& text){ emit supplierNameEdited(idx, text); emit monthChanged(); });
    auto bindChange = [this](QDoubleSpinBox* s) {
        connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double){ emit monthChanged(); });
    };
    bindChange(rw.previousBalance);
    bindChange(rw.purchases);
    bindChange(rw.payments);

    m_rowsLayout->addWidget(rw.row);
    m_rows.push_back(rw);
    setupRowContextMenu(m_rows.back(), idx);
    updateStyles();
}


void SupplierMonthCard::setupRowContextMenu(RowWidgets& rw, int rowIndex)
{
    auto bindMenu = [this, rowIndex](QWidget* w) {
        if (!w) return;
        w->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(w, &QWidget::customContextMenuRequested, this, [this, rowIndex, w](const QPoint& pos) {
            if (m_rows.size() <= 1) return;
            QMenu menu;
            const QString removeText = tr_auto_remove_supplier_e5807211();
            QAction* act = menu.addAction(removeText);
            if (menu.exec(w->mapToGlobal(pos)) == act)
                emit removeSupplierRequested(rowIndex);
        });
    };
    bindMenu(rw.row);
    bindMenu(rw.name);
    bindMenu(rw.previousBalance);
    bindMenu(rw.purchases);
    bindMenu(rw.totalDebt);
    bindMenu(rw.payments);
    bindMenu(rw.pctPurchases);
    bindMenu(rw.pctDebt);
    bindMenu(rw.balance);
}

bool SupplierMonthCard::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_header && ev->type() == QEvent::MouseButtonRelease) {
        toggleExpand();
        return true;
    }
    return QFrame::eventFilter(obj, ev);
}

void SupplierMonthCard::toggleExpand()
{
    setExpanded(!m_expanded);
}

void SupplierMonthCard::setAddButtonVisible(bool visible)
{
    if (!m_addBtn) return;
    m_addBtn->setVisible(visible);
    m_addBtn->setEnabled(visible);
}

void SupplierMonthCard::setExpanded(bool expand)
{
    if (m_expanded == expand) return;
    m_expanded = expand;
    if (m_chevron) m_chevron->setText(m_expanded ? "▲" : "▼");
    if (m_expanded) {
        m_content->setVisible(true);
        m_fullHeight = qMax(10, m_content->sizeHint().height());
        m_anim->stop();
        m_anim->setStartValue(0);
        m_anim->setEndValue(m_fullHeight);
        m_anim->start();
    } else {
        m_anim->stop();
        m_anim->setStartValue(m_content->maximumHeight());
        m_anim->setEndValue(0);
        m_anim->start();
        connect(m_anim, &QPropertyAnimation::finished, this, [this]{ if (!m_expanded) m_content->setVisible(false); }, Qt::UniqueConnection);
    }
}

int SupplierMonthCard::contentHeight() const
{
    return m_content ? m_content->maximumHeight() : 0;
}

void SupplierMonthCard::setContentHeight(int h)
{
    if (m_content) m_content->setMaximumHeight(h);
}

void SupplierMonthCard::setRowCount(int count)
{
    while (m_rows.size() < count)
        appendRow();
    while (m_rows.size() > count) {
        auto rw = m_rows.takeLast();
        delete rw.row;
    }
    QTimer::singleShot(0, this, [this]{
        m_fullHeight = qMax(10, m_content->sizeHint().height());
        if (m_expanded) m_content->setMaximumHeight(m_fullHeight);
    });
}

int SupplierMonthCard::rowCount() const { return m_rows.size(); }

QList<SupplierEntry> SupplierMonthCard::entries() const
{
    QList<SupplierEntry> out;
    for (const auto& r : m_rows) {
        SupplierEntry e;
        e.name = r.name->text().trimmed();
        e.previousBalance = r.previousBalance->value();
        e.purchases = r.purchases->value();
        e.totalDebt = r.totalDebt->value();
        e.payments = r.payments->value();
        out.append(e);
    }
    return out;
}

void SupplierMonthCard::setEntries(const QList<SupplierEntry>& entries)
{
    setRowCount(qMax(1, entries.size()));
    for (int i = 0; i < m_rows.size(); ++i) {
        const SupplierEntry e = i < entries.size() ? entries[i] : SupplierEntry{};
        auto& r = m_rows[i];
        r.name->setText(e.name);
        r.previousBalance->setValue(e.previousBalance);
        r.purchases->setValue(e.purchases);
        r.totalDebt->setValue(e.totalDebt > 0.0 ? e.totalDebt : (e.previousBalance + e.purchases));
        r.payments->setValue(e.payments);
    }
}

void SupplierMonthCard::refreshComputedValues(const QList<SupplierEntry>* previousMonthEntries)
{
    for (int i = 0; i < m_rows.size(); ++i) {
        auto& r = m_rows[i];
        if (previousMonthEntries && i < previousMonthEntries->size()) {
            const double carry = previousMonthEntries->at(i).supplierBalance();
            if (m_monthIndex != 0)
                r.previousBalance->setValue(carry);
        }
        const double debt = r.previousBalance->value() + r.purchases->value();
        r.totalDebt->setValue(debt);
        const double pctPurch = r.purchases->value() > 0.0 ? (r.payments->value() / r.purchases->value()) * 100.0 : 0.0;
        const double pctDebt = debt > 0.0 ? (r.payments->value() / debt) * 100.0 : 0.0;
        const double bal = debt - r.payments->value();
        r.pctPurchases->setText(QString::number(pctPurch, 'f', 2) + "%");
        r.pctDebt->setText(QString::number(pctDebt, 'f', 2) + "%");
        r.balance->setText(formatMoneyText(bal));
    }
    if (m_expanded) {
        QTimer::singleShot(0, this, [this]{
            m_fullHeight = qMax(10, m_content->sizeHint().height());
            m_content->setMaximumHeight(m_fullHeight);
        });
    }
}

void SupplierMonthCard::setNamesFrom(const QStringList& names)
{
    setRowCount(names.size());
    for (int i = 0; i < names.size() && i < m_rows.size(); ++i) {
        if (m_rows[i].name->text().trimmed() != names[i].trimmed()) {
            QSignalBlocker blocker(m_rows[i].name);
            m_rows[i].name->setText(names[i].trimmed());
        }
    }
}

void SupplierMonthCard::clearAll()
{
    for (auto& r : m_rows) {
        r.name->clear();
        r.previousBalance->setValue(0.0);
        r.purchases->setValue(0.0);
        r.totalDebt->setValue(0.0);
        r.payments->setValue(0.0);
        r.pctPurchases->clear();
        r.pctDebt->clear();
        r.balance->clear();
    }
}

void SupplierMonthCard::updateStyles()
{
    setStyleSheet(g_lightMode ? kCardLight : kCardDark);
    if (m_header) m_header->setStyleSheet(g_lightMode ? kHdrLight : kHdrDark);
    if (m_content) m_content->setStyleSheet(g_lightMode ? kContentLight : kContentDark);
    const QString lineEdit = g_lightMode
        ? "QLineEdit{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:4px 8px;}"
        : "QLineEdit{background:#252d4a;color:#c8d0ed;border:1px solid #3a4268;border-radius:6px;padding:4px 8px;}";
    const QString spin = g_lightMode
        ? "QDoubleSpinBox{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:4px 8px;}"
        : "QDoubleSpinBox{background:#252d4a;color:#c8d0ed;border:1px solid #3a4268;border-radius:6px;padding:4px 8px;}";
    const QString roSpin = g_lightMode
        ? "QDoubleSpinBox{background:#f4f6fb;color:#1e2340;border:1px solid #d8deec;border-radius:6px;padding:4px 8px;}"
        : "QDoubleSpinBox{background:#1a1f38;color:#c8d0ed;border:1px solid #2b3258;border-radius:6px;padding:4px 8px;}";
    const QString resultSS = g_lightMode
        ? "QLabel{background:#eef2ff;color:#1e2340;border:1px solid #d8deec;border-radius:6px;padding:6px;font-weight:700;}"
        : "QLabel{background:#1a1f38;color:#c8d0ed;border:1px solid #2b3258;border-radius:6px;padding:6px;font-weight:700;}";
    const QString btn = g_lightMode
        ? "QPushButton{background:#4f86f7;color:white;border:none;border-radius:6px;padding:0 12px;font-weight:700;}"
        : "QPushButton{background:#4f86f7;color:white;border:none;border-radius:6px;padding:0 12px;font-weight:700;}";
    if (m_addBtn) m_addBtn->setStyleSheet(btn);
    if (m_monthLabel) m_monthLabel->setStyleSheet(g_lightMode ? kMonthLblLight : kMonthLblDark);
    if (m_chevron) m_chevron->setStyleSheet(g_lightMode ? kChevronLight : kChevronDark);
    for (auto& r : m_rows) {
        r.name->setStyleSheet(lineEdit);
        r.previousBalance->setStyleSheet(r.previousBalance->isReadOnly() ? roSpin : spin);
        r.purchases->setStyleSheet(spin);
        r.totalDebt->setStyleSheet(roSpin);
        r.payments->setStyleSheet(spin);
        const QString fieldLbl = g_lightMode
        ? "QLabel{background:#e9eefb;color:#1e2340;border:1px solid #d8deec;border-radius:6px;padding:2px 4px;font-weight:800;}"
        : "QLabel{background:#202745;color:#c8d0ed;border:1px solid #2b3258;border-radius:6px;padding:2px 4px;font-weight:800;}";
        for (QLabel* lbl : {r.nameLabel, r.previousBalanceLabel, r.purchasesLabel, r.totalDebtLabel, r.paymentsLabel, r.pctPurchasesLabel, r.pctDebtLabel, r.balanceLabel}) {
            if (lbl) lbl->setStyleSheet(fieldLbl);
        }
        r.pctPurchases->setStyleSheet(resultSS);
        r.pctDebt->setStyleSheet(resultSS);
        r.balance->setStyleSheet(resultSS);
    }
}

void SupplierMonthCard::applyTheme() { updateStyles(); }
void SupplierMonthCard::updateCurrencyPrefix()
{
    for (auto& r : m_rows) {
        r.previousBalance->updatePrefix();
        r.purchases->updatePrefix();
        r.totalDebt->updatePrefix();
        r.payments->updatePrefix();
    }
    refreshComputedValues(nullptr);
}

void SupplierMonthCard::retranslate()
{
    const QStringList months = monthNames();
    if (m_monthLabel) m_monthLabel->setText(months.value(m_monthIndex));
    if (m_addBtn) m_addBtn->setText(tr_auto_add_supplier_58130448());

    for (auto& r : m_rows) {
        if (r.nameLabel) r.nameLabel->setText(tr_auto_supplier_name_ac45e726());
        if (r.previousBalanceLabel) r.previousBalanceLabel->setText(tr_auto_previous_balance_d6da85a6());
        if (r.purchasesLabel) r.purchasesLabel->setText(tr_auto_purchases_eb5647b3());
        if (r.totalDebtLabel) r.totalDebtLabel->setText(tr_auto_total_debt_b9772183());
        if (r.paymentsLabel) r.paymentsLabel->setText(tr_auto_payments_726d1e53());
        if (r.pctPurchasesLabel) r.pctPurchasesLabel->setText(tr_auto_payment_of_purchases_81a9c0e3());
        if (r.pctDebtLabel) r.pctDebtLabel->setText(tr_auto_payment_of_debt_ba7e4d60());
        if (r.balanceLabel) r.balanceLabel->setText(tr_auto_supplier_balance_74852681());

        r.name->setPlaceholderText(tr_auto_supplier_name_ac45e726());
        r.previousBalance->setToolTip(tr_auto_previous_balance_d6da85a6());
        r.purchases->setToolTip(tr_auto_purchases_eb5647b3());
        r.totalDebt->setToolTip(tr_auto_total_debt_b9772183());
        r.payments->setToolTip(tr_auto_payments_726d1e53());
        r.pctPurchases->setToolTip(tr_auto_payment_of_monthly_purchases_9baf921e());
        r.pctDebt->setToolTip(tr_auto_payment_of_total_debt_444c6cc5());
        r.balance->setToolTip(tr_auto_supplier_balance_74852681());
    }
}

SuppliersWidget::SuppliersWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("suppliersRoot");
    setAttribute(Qt::WA_StyledBackground, true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    auto* container = new QWidget;
    auto* vl = new QVBoxLayout(container);
    vl->setContentsMargins(20,16,20,24);
    vl->setSpacing(10);

    auto* titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(10);
    m_title = new QLabel;
    m_graphBtn = new QToolButton;
    m_graphBtn->setObjectName("showGraphsBtn");
    m_graphBtn->setCursor(Qt::PointingHandCursor);
    m_graphBtn->setFixedHeight(34);
    titleRow->addWidget(m_title);
    titleRow->addStretch();
    titleRow->addWidget(m_graphBtn, 0, Qt::AlignRight);
    m_subtitle = new QLabel;
    m_subtitle->setWordWrap(true);
    vl->addLayout(titleRow);
    vl->addWidget(m_subtitle);
    connect(m_graphBtn, &QAbstractButton::clicked, this, &SuppliersWidget::onShowGraphs);

    for (int i = 0; i < 12; ++i) {
        m_cards[i] = new SupplierMonthCard(i, container);
        m_cards[i]->setAddButtonVisible(i == 0);
        connect(m_cards[i], &SupplierMonthCard::addSupplierRequested, this, &SuppliersWidget::onAddSupplierRequested);
        connect(m_cards[i], &SupplierMonthCard::supplierNameEdited, this, &SuppliersWidget::onSupplierNameEdited);
        connect(m_cards[i], &SupplierMonthCard::monthChanged, this, &SuppliersWidget::onMonthChanged);
        connect(m_cards[i], &SupplierMonthCard::removeSupplierRequested, this, &SuppliersWidget::onRemoveSupplierRequested);
        vl->addWidget(m_cards[i]);
    }
    vl->addStretch();
    m_scroll->setWidget(container);
    root->addWidget(m_scroll);
    ensureGlobalSupplierCount(1);
    retranslate();
    applyTheme();
    refreshAllComputedValues();
}

void SuppliersWidget::updateGraphButtonMenu()
{
    if (!m_graphBtn)
        return;
    if (QMenu* menu = m_graphBtn->menu()) {
        m_graphBtn->setMenu(nullptr);
        menu->deleteLater();
    }
    m_graphBtn->setPopupMode(QToolButton::DelayedPopup);
}

bool SuppliersWidget::showGraphSelectionForRequest(const ChartRequest& request)
{
    AppData data = collectData();
    data.calculate();

    ChartKind kind = request.kind;
    if (kind == ChartKind::Pie)
        kind = ChartKind::ComparePie;
    else if (kind == ChartKind::MetricBar || kind == ChartKind::RankedBar)
        kind = ChartKind::CompareBar;
    else if (kind == ChartKind::MetricLine)
        kind = ChartKind::CompareLine;
    if (kind != ChartKind::ComparePie &&
        kind != ChartKind::CompareBar &&
        kind != ChartKind::CompareLine &&
        kind != ChartKind::HorizontalBar &&
        kind != ChartKind::Candle) {
        kind = ChartKind::CompareBar;
    }

    const bool hasExistingSelection = !request.compareMetrics.isEmpty()
        || !request.months.isEmpty()
        || request.includeSummaryPoint
        || request.comparePieBaseMetric != M_COUNT
        || !request.title.trimmed().isEmpty();
    const ChartRequest* existing = hasExistingSelection ? &request : nullptr;
    SupplierGraphSelectionDialog dlg(kind, data, this, existing);
    if (dlg.exec() != QDialog::Accepted)
        return false;

    ChartRequest req;
    req.origin = ChartOrigin::Suppliers;
    req.kind = dlg.kind();
    req.compareMetrics = dlg.selectedMetrics();
    req.metricA = req.compareMetrics.value(0, M_SUPPLIER_BALANCE);
    req.metricB = req.compareMetrics.value(1, req.metricA);
    req.months = dlg.selectedMonths();
    req.comparePieBaseMetric = dlg.countAs100Metric();
    req.includeSummaryPoint = dlg.includeSummaryPoint();
    emit graphRequested(req);
    return true;
}

void SuppliersWidget::onShowGraphs()
{
    ChartRequest req;
    req.origin = ChartOrigin::Suppliers;
    req.kind = ChartKind::CompareBar;
    showGraphSelectionForRequest(req);
}

void SuppliersWidget::ensureGlobalSupplierCount(int count)
{
    count = qMax(1, count);
    for (int i = 0; i < 12; ++i)
        m_cards[i]->setRowCount(count);
}

QStringList SuppliersWidget::currentSupplierNames() const
{
    QStringList names;
    const auto entries = m_cards[0]->entries();
    for (const auto& e : entries)
        names << e.name;
    return names;
}

void SuppliersWidget::refreshAllComputedValues()
{
    for (int i = 0; i < 12; ++i) {
        if (i == 0) m_cards[i]->refreshComputedValues(nullptr);
        else {
            const auto prev = m_cards[i-1]->entries();
            m_cards[i]->refreshComputedValues(&prev);
        }
    }
}

void SuppliersWidget::onAddSupplierRequested()
{
    ensureGlobalSupplierCount(m_cards[0]->rowCount() + 1);
    refreshAllComputedValues();
    emit dataChanged();
}

void SuppliersWidget::onSupplierNameEdited(int rowIndex, const QString& name)
{
    Q_UNUSED(rowIndex);
    Q_UNUSED(name);
    QStringList names = currentSupplierNames();
    for (int i = 1; i < 12; ++i)
        m_cards[i]->setNamesFrom(names);
    refreshAllComputedValues();
    emit dataChanged();
}

void SuppliersWidget::onMonthChanged()
{
    refreshAllComputedValues();
    emit dataChanged();
}


void SuppliersWidget::onRemoveSupplierRequested(int rowIndex)
{
    const int currentCount = m_cards[0]->rowCount();
    if (currentCount <= 1 || rowIndex < 0 || rowIndex >= currentCount)
        return;

    for (int i = 0; i < 12; ++i) {
        auto entries = m_cards[i]->entries();
        if (rowIndex >= 0 && rowIndex < entries.size())
            entries.removeAt(rowIndex);
        if (entries.isEmpty())
            entries.append(SupplierEntry{});
        m_cards[i]->setEntries(entries);
    }
    ensureGlobalSupplierCount(currentCount - 1);
    refreshAllComputedValues();
    emit dataChanged();
}

AppData SuppliersWidget::collectData() const
{
    AppData d;
    for (int i = 0; i < 12; ++i) {
        d.supplierEntries[i] = m_cards[i]->entries();
        double purch = 0.0, pay = 0.0;
        QString first;
        for (const auto& e : d.supplierEntries[i]) {
            purch += e.purchases;
            pay += e.payments;
            if (first.isEmpty() && !e.name.trimmed().isEmpty()) first = e.name.trimmed();
        }
        d.suppliers[i].supplierName = first;
        d.suppliers[i].purchases = purch;
        d.suppliers[i].payments = pay;
    }
    return d;
}

void SuppliersWidget::setData(const AppData& data)
{
    bool hasAnySupplierEntries = false;
    int maxRows = 1;
    for (int i = 0; i < 12; ++i) {
        maxRows = qMax(maxRows, data.supplierEntries[i].size());
        if (!data.supplierEntries[i].isEmpty())
            hasAnySupplierEntries = true;
    }
    ensureGlobalSupplierCount(maxRows);
    if (hasAnySupplierEntries) {
        for (int i = 0; i < 12; ++i)
            m_cards[i]->setEntries(data.supplierEntries[i]);
    } else {
        for (int i = 0; i < 12; ++i) {
            QList<SupplierEntry> entries;
            SupplierEntry e;
            e.name = data.suppliers[i].supplierName;
            e.purchases = data.suppliers[i].purchases;
            e.payments = data.suppliers[i].payments;
            entries << e;
            m_cards[i]->setEntries(entries);
        }
    }
    refreshAllComputedValues();
}

void SuppliersWidget::clearData()
{
    ensureGlobalSupplierCount(1);
    for (int i = 0; i < 12; ++i)
        m_cards[i]->clearAll();
    refreshAllComputedValues();
    emit dataChanged();
}

void SuppliersWidget::updateCurrencyPrefix()
{
    for (int i = 0; i < 12; ++i)
        m_cards[i]->updateCurrencyPrefix();
    refreshAllComputedValues();
}

void SuppliersWidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kRootLight : kRootDark);
    if (m_scroll && m_scroll->viewport())
        m_scroll->viewport()->setStyleSheet(g_lightMode ? "background:#f4f6fb;" : "background:#0d1020;");
    if (m_title) m_title->setStyleSheet(g_lightMode ? "font-size:18px;font-weight:900;color:#1e2340;" : "font-size:18px;font-weight:900;color:#c8d0ed;");
    if (m_subtitle) m_subtitle->setStyleSheet(g_lightMode ? "color:#6b7280;" : "color:#8892b8;");
    if (m_graphBtn) {
        m_graphBtn->setStyleSheet(g_lightMode
            ? "QToolButton#showGraphsBtn{background:#4f86f7;color:white;border:none;border-radius:7px;padding:8px 14px;font-weight:700;}"
              "QToolButton#showGraphsBtn:hover{background:#5e91f8;}"
              "QToolButton#showGraphsBtn:pressed{background:#3a6fe0;}"
            : "QToolButton#showGraphsBtn{background:#4f86f7;color:white;border:none;border-radius:7px;padding:8px 14px;font-weight:700;}"
              "QToolButton#showGraphsBtn:hover{background:#5e91f8;}"
              "QToolButton#showGraphsBtn:pressed{background:#3a6fe0;}");
    }
    for (int i = 0; i < 12; ++i)
        m_cards[i]->applyTheme();
}

void SuppliersWidget::retranslate()
{
    if (m_title) m_title->setText(tr_auto_suppliers_7beff393());
    if (m_subtitle) m_subtitle->setText(tr_auto_each_row_tracks_one_supplier_across_all_mo_c594a14b());
    if (m_graphBtn) m_graphBtn->setText(tr_show_graphs_26cf20());
    updateGraphButtonMenu();
    for (int i = 0; i < 12; ++i)
        m_cards[i]->retranslate();
}
