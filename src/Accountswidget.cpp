#include "Accountswidget.h"
#include "translations.h"
#include "themebox.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMenu>
#include <QAction>
#include <QAbstractButton>
#include <QSignalBlocker>
#include <QMessageBox>
#include <QWheelEvent>
#include <QAbstractItemView>
#include <QDialog>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QAbstractSpinBox>
#include <QSpinBox>
#include <algorithm>

namespace {

constexpr int kAccountsPerPage = 60;

class NoWheelComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;

protected:
    void wheelEvent(QWheelEvent* event) override
    {
        event->ignore();
    }
};

class NoWheelDoubleSpinBox : public QDoubleSpinBox
{
public:
    using QDoubleSpinBox::QDoubleSpinBox;

protected:
    void wheelEvent(QWheelEvent* event) override
    {
        event->ignore();
    }
};

class NoWheelSpinBox : public QSpinBox
{
public:
    using QSpinBox::QSpinBox;

protected:
    void wheelEvent(QWheelEvent* event) override
    {
        event->ignore();
    }
};

static const char* kAccountsSSDark = R"(
QWidget#accountsRoot { background:#0d1020; }
QWidget#accountsHeader { background:#111526; border-bottom:1px solid #1a1f38; }
QWidget#accountsContainer { background:#0d1020; }
QScrollArea#accountsScroll { background:#0d1020; border:none; }
QLabel#accountsTitle { color:#c8d0ed; font-weight:900; font-size:18px; background:transparent; }
QLabel#accountsSubtitle { color:#5a6490; background:transparent; }
QLabel#accountsFilterLabel { color:#c8d0ed; background:transparent; font-weight:700; }
QLineEdit, QComboBox, QDoubleSpinBox {
    background:#252d4a; color:#c8d0ed; border:1px solid #3a4268; border-radius:5px; padding:5px 8px;
}
QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus { border-color:#4f86f7; }
QPushButton#addAccountBtn, QToolButton#showGraphsBtn, QPushButton#accountsPageBtn {
    background:#4f86f7; color:white; border:none; border-radius:7px; padding:8px 14px; font-weight:700;
}
QPushButton#addAccountBtn:hover, QToolButton#showGraphsBtn:hover, QPushButton#accountsPageBtn:hover { background:#5e91f8; }
QPushButton#addAccountBtn:pressed, QToolButton#showGraphsBtn:pressed, QPushButton#accountsPageBtn:pressed { background:#3a6fe0; }
QWidget#accountRow {
    background:#111526; border:1px solid #1f2742; border-radius:11px;
}
QLabel#accountCode {
    background:#252d4a; color:#9fbaff; border:1px solid #384466; border-radius:8px;
    padding:5px 10px; font-weight:900;
}
QLabel#accountName { color:#e6ebff; background:transparent; font-weight:900; font-size:15px; }
QLabel#accountInfo { color:#9aa6cc; background:transparent; font-weight:600; }
QLabel#accountsPageLabel { color:#c8d0ed; background:transparent; font-weight:800; padding:0 10px; }
QDoubleSpinBox#accountAmount { min-width:120px; padding:4px 8px; }
QPushButton#removeAccountBtn {
    background:#1a1f38; color:#e05c6a; border:1px solid #252b52; border-radius:6px; padding:6px 10px; font-weight:700;
}
QPushButton#removeAccountBtn:hover { background:#2c1530; }
QPushButton#accountsPageBtn:disabled { background:#252d4a; color:#59648a; }
QScrollArea { background:transparent; border:none; }
QScrollBar:vertical { background:#0d1020; width:8px; border-radius:4px; }
QScrollBar::handle:vertical { background:#2e3860; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#4f86f7; }
QMenu { background:#12152a; color:#e6ebff; border:1px solid #2e3455; }
QMenu::item:selected { background:#4f86f7; color:white; }
)";

static const char* kAccountsSSLight = R"(
QWidget#accountsRoot { background:#f4f6fb; }
QWidget#accountsHeader { background:#ffffff; border-bottom:1px solid #dde2f0; }
QWidget#accountsContainer { background:#f4f6fb; }
QScrollArea#accountsScroll { background:#f4f6fb; border:none; }
QLabel#accountsTitle { color:#1e2340; font-weight:900; font-size:18px; background:transparent; }
QLabel#accountsSubtitle { color:#6b7280; background:transparent; }
QLabel#accountsFilterLabel { color:#1e2340; background:transparent; font-weight:700; }
QLineEdit, QComboBox, QDoubleSpinBox {
    background:#ffffff; color:#1e2340; border:1px solid #cfd7ea; border-radius:5px; padding:5px 8px;
}
QLineEdit:focus, QComboBox:focus, QDoubleSpinBox:focus { border-color:#4f86f7; }
QPushButton#addAccountBtn, QToolButton#showGraphsBtn, QPushButton#accountsPageBtn {
    background:#4f86f7; color:white; border:none; border-radius:7px; padding:8px 14px; font-weight:700;
}
QPushButton#addAccountBtn:hover, QToolButton#showGraphsBtn:hover, QPushButton#accountsPageBtn:hover { background:#5e91f8; }
QPushButton#addAccountBtn:pressed, QToolButton#showGraphsBtn:pressed, QPushButton#accountsPageBtn:pressed { background:#3a6fe0; }
QWidget#accountRow {
    background:#ffffff; border:1px solid #dde2f0; border-radius:11px;
}
QLabel#accountCode {
    background:#eef3ff; color:#1d4ed8; border:1px solid #cfdbff; border-radius:8px;
    padding:5px 10px; font-weight:900;
}
QLabel#accountName { color:#1e2340; background:transparent; font-weight:900; font-size:15px; }
QLabel#accountInfo { color:#5f6b83; background:transparent; font-weight:600; }
QLabel#accountsPageLabel { color:#1e2340; background:transparent; font-weight:800; padding:0 10px; }
QDoubleSpinBox#accountAmount { min-width:120px; padding:4px 8px; }
QPushButton#removeAccountBtn {
    background:#ffffff; color:#c0392b; border:1px solid #d9e0ef; border-radius:6px; padding:6px 10px; font-weight:700;
}
QPushButton#removeAccountBtn:hover { background:#fdf2f2; }
QPushButton#accountsPageBtn:disabled { background:#e5e9f3; color:#9aa3b8; }
QScrollArea { background:transparent; border:none; }
QScrollBar:vertical { background:#f4f6fb; width:8px; border-radius:4px; }
QScrollBar::handle:vertical { background:#c8d0ed; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#4f86f7; }
QMenu { background:#ffffff; color:#1e2340; border:1px solid #d9e0ef; }
QMenu::item:selected { background:#4f86f7; color:white; }
)";

static QString dialogStyle()
{
    const QString base = g_lightMode
        ? QStringLiteral("QDialog{background:#f4f6fb;} QLabel{color:#1e2340;} QLineEdit,QComboBox,QDoubleSpinBox{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:6px 8px;} QPushButton{background:#4f86f7;color:white;border:none;border-radius:7px;padding:8px 18px;font-weight:700;} QPushButton#cancelBtn{background:#eef0fa;color:#5a6490;border:1px solid #dde2f0;} QCheckBox{color:#1e2340;font-weight:700;} QCheckBox::indicator{width:42px;height:22px;border-radius:11px;border:1px solid #b8c2d8;background:#d1d5db;} QCheckBox::indicator:checked{background:#4f86f7;border:1px solid #4f86f7;}")
        : QStringLiteral("QDialog{background:#12152a;} QLabel{color:#e6ebff;} QLineEdit,QComboBox,QDoubleSpinBox{background:#252d4a;color:#c8d0ed;border:1px solid #3a4268;border-radius:6px;padding:6px 8px;} QPushButton{background:#4f86f7;color:white;border:none;border-radius:7px;padding:8px 18px;font-weight:700;} QPushButton#cancelBtn{background:#1e2340;color:#c8d0ed;border:1px solid #2e3455;} QCheckBox{color:#e6ebff;font-weight:700;} QCheckBox::indicator{width:42px;height:22px;border-radius:11px;border:1px solid #384466;background:#1e2340;} QCheckBox::indicator:checked{background:#4f86f7;border:1px solid #4f86f7;}");
    return base;
}

static QString settlementText(bool allowed)
{
    return allowed ? T("Settlement: On", "التسوية: مفعلة")
                   : T("Settlement: Off", "التسوية: متوقفة");
}

static QString codeDisplay(const AccountItem& item)
{
    const QString code = item.code.trimmed();
    return code.isEmpty() ? T("No code", "بدون رمز") : code;
}

static QString nameDisplay(const AccountItem& item)
{
    const QString name = item.name.trimmed();
    return name.isEmpty() ? T("Unnamed account", "حساب بدون اسم") : name;
}

class AccountGraphSelectionDialog : public QDialog
{
public:
    explicit AccountGraphSelectionDialog(QWidget* parent = nullptr, const ChartRequest* existing = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(tr_show_graphs_26cf20());
        setModal(true);
        setMinimumWidth(520);
        setLayoutDirection(appLayoutDirection());
        setStyleSheet(g_lightMode
            ? QStringLiteral("QDialog{background:#f4f6fb;} QLabel{color:#1e2340;background:transparent;} QComboBox,QSpinBox,QPushButton{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:7px;padding:7px 12px;font-weight:700;} QPushButton{background:#4f86f7;color:#ffffff;border:none;} QPushButton#cancelBtn{background:#eef0fa;color:#5a6490;border:1px solid #dde2f0;} QComboBox:hover,QSpinBox:hover{background:#eef0fa;} QListView{background:#ffffff;color:#1e2340;selection-background-color:#eef0fa;selection-color:#1e2340;border:1px solid #dde2f0;} QListView::item{padding:6px 8px;}")
            : QStringLiteral("QDialog{background:#12152a;} QLabel{color:#e6ebff;background:transparent;} QComboBox,QSpinBox,QPushButton{background:#1a1f38;color:#e7ecff;border:1px solid #343c63;border-radius:7px;padding:7px 12px;font-weight:700;} QPushButton{background:#4f86f7;color:#ffffff;border:none;} QPushButton#cancelBtn{background:#1e2340;color:#c8d0ed;border:1px solid #2e3455;} QComboBox:hover,QSpinBox:hover{background:#1e2445;} QListView{background:#1a1f38;color:#c8d0ed;selection-background-color:#4f86f7;selection-color:#ffffff;border:1px solid #252b52;} QListView::item{padding:6px 8px;}")
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

        auto* chartTypeLabel = new QLabel(tr_chart_type_bd42b2(), this);
        chartTypeLabel->setStyleSheet(QStringLiteral("font-weight:800;"));
        root->addWidget(chartTypeLabel);

        m_chartTypeCombo = new NoWheelComboBox(this);
        styleComboPopup(m_chartTypeCombo);
        m_chartTypeCombo->addItem(tr_pie_chart_9d4e04(), int(ChartKind::Pie));
        m_chartTypeCombo->addItem(tr_bar_chart_a5f324(), int(ChartKind::RankedBar));
        m_chartTypeCombo->addItem(T("Horizontal bar", "شريط أفقي"), int(ChartKind::HorizontalBar));
        m_chartTypeCombo->addItem(tr_line_chart_932796(), int(ChartKind::MetricLine));
        m_chartTypeCombo->addItem(tr_candle_chart_f7a9c2(), int(ChartKind::Candle));
        const int barIndex = m_chartTypeCombo->findData(int(ChartKind::RankedBar));
        if (barIndex >= 0) m_chartTypeCombo->setCurrentIndex(barIndex);
        if (existing) {
            ChartKind presetKind = existing->kind;
            if (presetKind == ChartKind::HorizontalBar)
                presetKind = ChartKind::HorizontalBar;
            else if (presetKind == ChartKind::ComparePie)
                presetKind = ChartKind::Pie;
            else if (presetKind == ChartKind::CompareBar || presetKind == ChartKind::MetricBar)
                presetKind = ChartKind::RankedBar;
            else if (presetKind == ChartKind::CompareLine)
                presetKind = ChartKind::MetricLine;
            const int presetKindIndex = m_chartTypeCombo->findData(int(presetKind));
            if (presetKindIndex >= 0)
                m_chartTypeCombo->setCurrentIndex(presetKindIndex);
        }
        root->addWidget(m_chartTypeCombo);

        auto* title = new QLabel(T("Top accounts", "أعلى الحسابات"), this);
        title->setStyleSheet(QStringLiteral("font-weight:800;font-size:15px;"));
        root->addWidget(title);

        auto* form = new QFormLayout;
        form->setLabelAlignment(appTextAlign());
        form->setFormAlignment(Qt::AlignTop);
        form->setHorizontalSpacing(14);
        form->setVerticalSpacing(10);

        m_typeCombo = new NoWheelComboBox(this);
        styleComboPopup(m_typeCombo);
        m_typeCombo->addItem(accountTypeFilterDisplayName(AccountTypeFilter::All), int(AccountTypeFilter::All));
        for (AccountType type : accountTypesInUiOrder())
            m_typeCombo->addItem(accountTypeDisplayName(type), int(accountTypeFilterFromType(type)));

        m_countSpin = new NoWheelSpinBox(this);
        m_countSpin->setRange(1, 1000);
        m_countSpin->setValue(10);
        if (existing) {
            const int typeIndex = m_typeCombo->findData(int(existing->accountFilter));
            if (typeIndex >= 0)
                m_typeCombo->setCurrentIndex(typeIndex);
            if (existing->topAccountCount > 0)
                m_countSpin->setValue(qBound(1, existing->topAccountCount, 1000));
        }
        m_countSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_countSpin->setAlignment(appTextAlign() | Qt::AlignVCenter);

        form->addRow(T("Account type", "نوع الحساب"), m_typeCombo);
        form->addRow(T("How many accounts", "عدد الحسابات"), m_countSpin);
        root->addLayout(form);

        auto* note = new QLabel(T("The chart will show the highest accounts by amount for the selected account type.", "سيعرض الرسم أعلى الحسابات حسب المبلغ لنوع الحساب المحدد."), this);
        note->setWordWrap(true);
        note->setStyleSheet(g_lightMode ? QStringLiteral("color:#6b7280;font-weight:600;") : QStringLiteral("color:#9aa6cc;font-weight:600;"));
        root->addWidget(note);

        auto* buttons = new QHBoxLayout;
        buttons->addStretch();
        auto* cancel = new QPushButton(tr_cancel_8d40ef(), this);
        cancel->setObjectName("cancelBtn");
        auto* ok = new QPushButton(tr_show_graphs_26cf20(), this);
        buttons->addWidget(cancel);
        buttons->addWidget(ok);
        root->addLayout(buttons);

        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    }

    ChartKind chartKind() const
    {
        return m_chartTypeCombo ? static_cast<ChartKind>(m_chartTypeCombo->currentData().toInt()) : ChartKind::RankedBar;
    }

    AccountTypeFilter accountFilter() const
    {
        return m_typeCombo ? static_cast<AccountTypeFilter>(m_typeCombo->currentData().toInt()) : AccountTypeFilter::All;
    }

    int topCount() const
    {
        return m_countSpin ? qMax(1, m_countSpin->value()) : 10;
    }

private:
    QComboBox* m_chartTypeCombo{};
    QComboBox* m_typeCombo{};
    QSpinBox* m_countSpin{};
};

}

Accountswidget::Accountswidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("accountsRoot");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget;
    header->setObjectName("accountsHeader");
    auto* hl = new QVBoxLayout(header);
    hl->setContentsMargins(20, 16, 20, 14);
    hl->setSpacing(10);

    m_title = new QLabel;
    m_title->setObjectName("accountsTitle");
    m_subtitle = new QLabel;
    m_subtitle->setObjectName("accountsSubtitle");
    m_subtitle->setWordWrap(true);
    hl->addWidget(m_title);
    hl->addWidget(m_subtitle);

    auto* filterRow1 = new QHBoxLayout;
    filterRow1->setSpacing(10);

    m_searchLabel = new QLabel;
    m_searchLabel->setObjectName("accountsFilterLabel");
    m_searchEdit = new QLineEdit;
    m_searchEdit->setMinimumWidth(240);

    m_sortLabel = new QLabel;
    m_sortLabel->setObjectName("accountsFilterLabel");
    m_sortCombo = new NoWheelComboBox;

    m_typeLabel = new QLabel;
    m_typeLabel->setObjectName("accountsFilterLabel");
    m_typeFilterCombo = new NoWheelComboBox;
    m_typeFilterCombo->setMinimumWidth(190);

    filterRow1->addWidget(m_searchLabel);
    filterRow1->addWidget(m_searchEdit, 1);
    filterRow1->addSpacing(8);
    filterRow1->addWidget(m_sortLabel);
    filterRow1->addWidget(m_sortCombo);
    filterRow1->addSpacing(8);
    filterRow1->addWidget(m_typeLabel);
    filterRow1->addWidget(m_typeFilterCombo);
    hl->addLayout(filterRow1);

    auto* filterRow2 = new QHBoxLayout;
    filterRow2->setSpacing(10);

    m_settlementLabel = new QLabel;
    m_settlementLabel->setObjectName("accountsFilterLabel");
    m_settlementFilterCombo = new NoWheelComboBox;

    m_currencyLabel = new QLabel;
    m_currencyLabel->setObjectName("accountsFilterLabel");
    m_currencyFilterCombo = new NoWheelComboBox;
    m_currencyFilterCombo->setMinimumWidth(120);

    m_addBtn = new QPushButton;
    m_addBtn->setObjectName("addAccountBtn");

    m_graphBtn = new QToolButton;
    m_graphBtn->setObjectName("showGraphsBtn");

    filterRow2->addWidget(m_settlementLabel);
    filterRow2->addWidget(m_settlementFilterCombo);
    filterRow2->addSpacing(8);
    filterRow2->addWidget(m_currencyLabel);
    filterRow2->addWidget(m_currencyFilterCombo);
    filterRow2->addStretch();
    filterRow2->addWidget(m_addBtn);
    filterRow2->addWidget(m_graphBtn);
    hl->addLayout(filterRow2);

    root->addWidget(header);

    m_scroll = new QScrollArea;
    m_scroll->setObjectName("accountsScroll");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget;
    m_container->setObjectName("accountsContainer");
    m_container->setAttribute(Qt::WA_StyledBackground, true);
    auto* vl = new QVBoxLayout(m_container);
    vl->setContentsMargins(20, 18, 20, 20);
    vl->setSpacing(10);
    m_rowsLayout = vl;

    m_empty = new QLabel;
    m_empty->setAlignment(Qt::AlignCenter);
    m_empty->setStyleSheet("background:transparent; color:#5a6490; font-weight:600;");
    vl->addWidget(m_empty);
    vl->addStretch();

    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll, 1);

    m_paginationBar = new QWidget;
    m_paginationBar->setObjectName("accountsHeader");
    auto* pagerLayout = new QHBoxLayout(m_paginationBar);
    pagerLayout->setContentsMargins(20, 8, 20, 12);
    pagerLayout->setSpacing(10);
    pagerLayout->addStretch();

    m_prevPageBtn = new QPushButton;
    m_prevPageBtn->setObjectName("accountsPageBtn");
    m_pageLabel = new QLabel;
    m_pageLabel->setObjectName("accountsPageLabel");
    m_pageLabel->setAlignment(Qt::AlignCenter);
    m_nextPageBtn = new QPushButton;
    m_nextPageBtn->setObjectName("accountsPageBtn");

    pagerLayout->addWidget(m_prevPageBtn);
    pagerLayout->addWidget(m_pageLabel);
    pagerLayout->addWidget(m_nextPageBtn);
    pagerLayout->addStretch();
    root->addWidget(m_paginationBar);

    connect(m_addBtn, &QPushButton::clicked, this, &Accountswidget::onAddAccount);
    connect(m_graphBtn, &QAbstractButton::clicked, this, &Accountswidget::onShowGraphs);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString&) { onFilterChanged(); });
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { sortAndRebuild(); });
    connect(m_typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { onFilterChanged(); });
    connect(m_settlementFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { onFilterChanged(); });
    connect(m_currencyFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { onFilterChanged(); });
    connect(m_prevPageBtn, &QPushButton::clicked, this, [this]() {
        syncVisibleRowsToAccounts();
        if (m_currentPage > 0) {
            --m_currentPage;
            applyFilters();
        }
    });
    connect(m_nextPageBtn, &QPushButton::clicked, this, [this]() {
        syncVisibleRowsToAccounts();
        const int pages = totalPagesForCount(filteredAccountIndexes().size());
        if (m_currentPage + 1 < pages) {
            ++m_currentPage;
            applyFilters();
        }
    });

    retranslate();
    applyTheme();
    updateGraphButtonMenu();
}

void Accountswidget::populateTypeCombo(QComboBox* combo, bool includeAll) const
{
    if (!combo)
        return;
    combo->clear();
    if (includeAll)
        combo->addItem(accountTypeFilterDisplayName(AccountTypeFilter::All), int(AccountTypeFilter::All));
    for (AccountType type : accountTypesInUiOrder())
        combo->addItem(accountTypeDisplayName(type), int(type));
}

AccountType Accountswidget::accountTypeFromCombo(const QComboBox* combo) const
{
    if (!combo)
        return AccountType::Payable;
    return static_cast<AccountType>(combo->currentData().toInt());
}

AccountTypeFilter Accountswidget::currentTypeFilter() const
{
    if (!m_typeFilterCombo)
        return AccountTypeFilter::All;
    return static_cast<AccountTypeFilter>(m_typeFilterCombo->currentData().toInt());
}

void Accountswidget::refreshCurrencyFilter()
{
    if (!m_currencyFilterCombo)
        return;

    const QString old = m_currencyFilterCombo->currentData().toString();
    QSignalBlocker blocker(m_currencyFilterCombo);
    m_currencyFilterCombo->clear();
    m_currencyFilterCombo->addItem(tr_all_b4d286(), QString());

    QStringList currencies;
    currencies << QStringLiteral("USD") << QStringLiteral("IQD");
    for (const auto& item : m_accounts) {
        const QString cur = item.currency.trimmed();
        if (!cur.isEmpty() && !currencies.contains(cur, Qt::CaseInsensitive))
            currencies << cur;
    }
    for (const QString& cur : currencies)
        m_currencyFilterCombo->addItem(cur, cur);

    const int idx = m_currencyFilterCombo->findData(old);
    if (idx >= 0)
        m_currencyFilterCombo->setCurrentIndex(idx);
}

ChartKind Accountswidget::normalizeAccountChartKind(ChartKind kind) const
{
    switch (kind) {
    case ChartKind::Pie:
    case ChartKind::Candle:
    case ChartKind::RankedBar:
    case ChartKind::MetricLine:
    case ChartKind::HorizontalBar:
        return kind;
    case ChartKind::MetricBar:
    case ChartKind::CompareBar:
        return ChartKind::RankedBar;
    case ChartKind::CompareLine:
        return ChartKind::MetricLine;
    case ChartKind::ComparePie:
        return ChartKind::Pie;
    }
    return ChartKind::RankedBar;
}

bool Accountswidget::openGraphDialog(ChartRequest& request, const ChartRequest* existing)
{
    AccountGraphSelectionDialog dialog(this, existing);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    request.kind = normalizeAccountChartKind(dialog.chartKind());
    request.metricA = M_EXPENSES;
    request.metricB = M_EXPENSES;
    request.compareMetrics.clear();
    request.axisMetricsAuto = true;
    request.accountFilter = dialog.accountFilter();
    request.topAccountCount = dialog.topCount();
    request.origin = ChartOrigin::Accounts;
    request.title = T("Top accounts", "أعلى الحسابات") + QStringLiteral(" — ")
        + accountTypeFilterDisplayName(request.accountFilter)
        + QStringLiteral(" — ") + T("Top %1", "أعلى %1").arg(request.topAccountCount);
    return true;
}

void Accountswidget::updateGraphButtonMenu()
{
    if (!m_graphBtn)
        return;
    if (QMenu* menu = m_graphBtn->menu()) {
        m_graphBtn->setMenu(nullptr);
        menu->deleteLater();
    }
    m_graphBtn->setPopupMode(QToolButton::DelayedPopup);
}

void Accountswidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kAccountsSSLight : kAccountsSSDark);
    if (m_container) {
        m_container->setAttribute(Qt::WA_StyledBackground, true);
        m_container->setStyleSheet(g_lightMode ? "background:#f4f6fb;" : "background:#0d1020;");
    }
    applyFilters();
}

void Accountswidget::retranslate()
{
    if (m_title) m_title->setText(tr_expenses_13597e());
    if (m_subtitle) m_subtitle->setText(T("Manage expense accounts here. Use Add to create an account with code, name, type, settlement, currency, and amount.", "أدر حسابات المصروفات هنا. استخدم إضافة لإنشاء حساب مع الرمز والاسم والنوع والتسوية والعملة والمبلغ."));
    if (m_searchLabel) m_searchLabel->setText(T("Search", "بحث"));
    if (m_searchEdit) m_searchEdit->setPlaceholderText(T("Search code, account name, type, settlement, currency, or amount", "ابحث عن الرمز أو اسم الحساب أو النوع أو التسوية أو العملة أو المبلغ"));
    if (m_sortLabel) m_sortLabel->setText(tr_sort_0f56a7());
    if (m_typeLabel) m_typeLabel->setText(T("Type", "النوع"));
    if (m_settlementLabel) m_settlementLabel->setText(T("Settlement", "التسوية"));
    if (m_currencyLabel) m_currencyLabel->setText(tr_currency_88f072());
    if (m_addBtn) m_addBtn->setText(tr_add_a98dbf());
    if (m_graphBtn) m_graphBtn->setText(tr_show_graphs_26cf20());
    if (m_prevPageBtn) m_prevPageBtn->setText(T("Previous", "السابق"));
    if (m_nextPageBtn) m_nextPageBtn->setText(T("Next", "التالي"));

    if (m_sortCombo) {
        const int old = m_sortCombo->currentIndex();
        QSignalBlocker blocker(m_sortCombo);
        m_sortCombo->clear();
        m_sortCombo->addItem(tr_ascending_c0fe46());
        m_sortCombo->addItem(tr_descending_d6045b());
        m_sortCombo->setCurrentIndex(qBound(0, old, 1));
    }

    if (m_typeFilterCombo) {
        const int oldData = m_typeFilterCombo->count() > 0
            ? m_typeFilterCombo->currentData().toInt()
            : int(AccountTypeFilter::All);
        QSignalBlocker blocker(m_typeFilterCombo);
        populateTypeCombo(m_typeFilterCombo, true);
        const int idx = m_typeFilterCombo->findData(oldData);
        if (idx >= 0) m_typeFilterCombo->setCurrentIndex(idx);
    }

    if (m_settlementFilterCombo) {
        const int oldData = m_settlementFilterCombo->count() > 0
            ? m_settlementFilterCombo->currentData().toInt()
            : -1;
        QSignalBlocker blocker(m_settlementFilterCombo);
        m_settlementFilterCombo->clear();
        m_settlementFilterCombo->addItem(tr_all_b4d286(), -1);
        m_settlementFilterCombo->addItem(T("Allowed", "مسموحة"), 1);
        m_settlementFilterCombo->addItem(T("Not allowed", "غير مسموحة"), 0);
        const int idx = m_settlementFilterCombo->findData(oldData);
        if (idx >= 0) m_settlementFilterCombo->setCurrentIndex(idx);
    }

    refreshCurrencyFilter();

    if (m_empty)
        m_empty->setText(tr_no_accounts_added_yet_b3f1b8());

    for (auto& row : m_rows) {
        if (row.code) row.code->setText(codeDisplay(row.item));
        if (row.name) row.name->setText(nameDisplay(row.item));
        if (row.type) row.type->setText(T("Type: ", "النوع: ") + accountTypeDisplayName(row.item.type));
        if (row.settlement) row.settlement->setText(settlementText(row.item.allowSettlement));
        if (row.currency) row.currency->setText(T("Currency: ", "العملة: ") + (row.item.currency.trimmed().isEmpty() ? QStringLiteral("-") : row.item.currency.trimmed()));
        if (row.amountLabel) row.amountLabel->setText(T("Amount:", "المبلغ:"));
        if (row.removeBtn) row.removeBtn->setText(tr_remove_c3a712());
        updateRowAlignment(row);
    }

    updatePaginationControls(filteredAccountIndexes().size());
    updateGraphButtonMenu();
    applyFilters();
}

bool Accountswidget::hasDuplicateAccount(const AccountItem& item, int ignoreIndex) const
{
    const QString codeKey = item.code.trimmed().toCaseFolded();
    const QString nameKey = item.name.trimmed().toCaseFolded();

    for (int i = 0; i < m_accounts.size(); ++i) {
        if (i == ignoreIndex)
            continue;

        const AccountItem& existing = m_accounts.at(i);
        const QString existingCode = existing.code.trimmed().toCaseFolded();
        const QString existingName = existing.name.trimmed().toCaseFolded();

        if (!codeKey.isEmpty() && !existingCode.isEmpty() && codeKey == existingCode)
            return true;
        if (!nameKey.isEmpty() && !existingName.isEmpty() && nameKey == existingName)
            return true;
    }
    return false;
}

int Accountswidget::totalPagesForCount(int count) const
{
    if (count <= 0)
        return 1;
    return (count + kAccountsPerPage - 1) / kAccountsPerPage;
}

void Accountswidget::updatePaginationControls(int filteredCount)
{
    const int pages = totalPagesForCount(filteredCount);
    if (m_currentPage >= pages)
        m_currentPage = qMax(0, pages - 1);
    if (m_currentPage < 0)
        m_currentPage = 0;

    if (m_pageLabel) {
        m_pageLabel->setText(T("Page %1 of %2 — %3 accounts", "الصفحة %1 من %2 — %3 حساب")
            .arg(m_currentPage + 1)
            .arg(pages)
            .arg(filteredCount));
    }

    if (m_prevPageBtn)
        m_prevPageBtn->setEnabled(m_currentPage > 0);
    if (m_nextPageBtn)
        m_nextPageBtn->setEnabled(m_currentPage + 1 < pages);
    if (m_paginationBar)
        m_paginationBar->setVisible(filteredCount > kAccountsPerPage);
}

void Accountswidget::updateRowAlignment(RowWidgets& row)
{
    const bool ar = isArabic();
    const Qt::LayoutDirection dir = ar ? Qt::RightToLeft : Qt::LeftToRight;
    const Qt::Alignment side = ar ? Qt::AlignRight : Qt::AlignLeft;

    if (row.row)
        row.row->setLayoutDirection(dir);
    if (auto* layout = row.row ? qobject_cast<QHBoxLayout*>(row.row->layout()) : nullptr)
        layout->setDirection(ar ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);

    const Qt::Alignment textAlign = side | Qt::AlignVCenter;
    if (row.code) {
        row.code->setLayoutDirection(dir);
        row.code->setAlignment(Qt::AlignCenter);
    }
    if (row.name) {
        row.name->setLayoutDirection(dir);
        row.name->setAlignment(textAlign);
    }
    if (row.type) {
        row.type->setLayoutDirection(dir);
        row.type->setAlignment(textAlign);
    }
    if (row.settlement) {
        row.settlement->setLayoutDirection(dir);
        row.settlement->setAlignment(textAlign);
    }
    if (row.currency) {
        row.currency->setLayoutDirection(dir);
        row.currency->setAlignment(textAlign);
    }
    if (row.amountLabel) {
        row.amountLabel->setLayoutDirection(dir);
        row.amountLabel->setAlignment(textAlign);
    }
    if (row.amount) {
        row.amount->setLayoutDirection(dir);
        row.amount->setAlignment(textAlign);
    }
}

void Accountswidget::clearRenderedRows()
{
    while (!m_rows.isEmpty()) {
        auto row = m_rows.takeLast();
        if (row.row)
            row.row->deleteLater();
    }
}

void Accountswidget::syncVisibleRowsToAccounts()
{
    for (const auto& row : m_rows) {
        if (row.sourceIndex >= 0 && row.sourceIndex < m_accounts.size()) {
            AccountItem item = row.item;
            if (row.amount)
                item.amount = row.amount->value();
            m_accounts[row.sourceIndex] = item;
        }
    }
}

void Accountswidget::addRow(const AccountItem& rawItem)
{
    syncVisibleRowsToAccounts();

    AccountItem item = rawItem;
    if (item.currency.trimmed().isEmpty())
        item.currency = (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"));

    m_accounts.append(item);
    refreshCurrencyFilter();

    const int pages = totalPagesForCount(filteredAccountIndexes().size());
    m_currentPage = qMax(0, pages - 1);
    applyFilters();
}

void Accountswidget::addRenderedRow(const AccountItem& item, int sourceIndex)
{
    if (m_empty)
        m_empty->hide();

    auto* rowW = new QWidget(m_container);
    rowW->setObjectName("accountRow");
    rowW->setLayoutDirection(appLayoutDirection());
    rowW->setContextMenuPolicy(Qt::CustomContextMenu);
    auto* hl = new QHBoxLayout(rowW);
    hl->setDirection(isArabic() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);
    hl->setContentsMargins(14, 12, 14, 12);
    hl->setSpacing(12);

    auto* code = new QLabel(codeDisplay(item), rowW);
    code->setObjectName("accountCode");
    code->setMinimumWidth(80);
    code->setAlignment(Qt::AlignCenter);

    auto* nameBlock = new QWidget(rowW);
    nameBlock->setLayoutDirection(appLayoutDirection());
    auto* nameLayout = new QVBoxLayout(nameBlock);
    nameLayout->setContentsMargins(0, 0, 0, 0);
    nameLayout->setSpacing(4);

    auto* name = new QLabel(nameDisplay(item), rowW);
    name->setObjectName("accountName");
    name->setLayoutDirection(appLayoutDirection());
    name->setAlignment(appTextAlign() | Qt::AlignVCenter);

    auto* infoRow = new QHBoxLayout;
    infoRow->setDirection(isArabic() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);
    infoRow->setContentsMargins(0, 0, 0, 0);
    infoRow->setSpacing(14);

    auto* type = new QLabel(T("Type: ", "النوع: ") + accountTypeDisplayName(item.type), rowW);
    type->setObjectName("accountInfo");
    auto* settlement = new QLabel(settlementText(item.allowSettlement), rowW);
    settlement->setObjectName("accountInfo");
    auto* currency = new QLabel(T("Currency: ", "العملة: ") + item.currency.trimmed(), rowW);
    currency->setObjectName("accountInfo");
    auto* amountLabel = new QLabel(T("Amount:", "المبلغ:"), rowW);
    amountLabel->setObjectName("accountInfo");
    auto* amount = new NoWheelDoubleSpinBox(rowW);
    amount->setObjectName("accountAmount");
    amount->setRange(-999999999999.99, 999999999999.99);
    amount->setDecimals(2);
    amount->setSingleStep(1000.0);
    amount->setValue(item.amount);
    amount->setKeyboardTracking(false);
    amount->setGroupSeparatorShown(true);
    amount->setButtonSymbols(QAbstractSpinBox::NoButtons);
    amount->setAlignment(appTextAlign() | Qt::AlignVCenter);

    infoRow->addWidget(type);
    infoRow->addWidget(settlement);
    infoRow->addWidget(currency);
    infoRow->addWidget(amountLabel);
    infoRow->addWidget(amount);
    infoRow->addStretch();

    nameLayout->addWidget(name);
    nameLayout->addLayout(infoRow);

    auto* removeBtn = new QPushButton(tr_remove_c3a712(), rowW);
    removeBtn->setObjectName("removeAccountBtn");
    removeBtn->setFixedWidth(90);

    hl->addWidget(code, 0);
    hl->addWidget(nameBlock, 1);
    hl->addWidget(removeBtn, 0, isArabic() ? Qt::AlignLeft : Qt::AlignRight);

    RowWidgets widgets;
    widgets.row = rowW;
    widgets.code = code;
    widgets.name = name;
    widgets.type = type;
    widgets.settlement = settlement;
    widgets.currency = currency;
    widgets.amountLabel = amountLabel;
    widgets.amount = amount;
    widgets.removeBtn = removeBtn;
    widgets.item = item;
    widgets.sourceIndex = sourceIndex;
    updateRowAlignment(widgets);
    m_rows.append(widgets);

    connect(removeBtn, &QPushButton::clicked, this, &Accountswidget::onRemoveRow);
    connect(amount, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, amount](double value) {
        for (auto& row : m_rows) {
            if (row.amount == amount) {
                row.item.amount = value;
                if (row.sourceIndex >= 0 && row.sourceIndex < m_accounts.size())
                    m_accounts[row.sourceIndex].amount = value;
                break;
            }
        }
    });
    auto attachAccountContextMenu = [this, rowW](QWidget* target) {
        if (!target)
            return;
        target->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(target, &QWidget::customContextMenuRequested, this, [this, rowW, target](const QPoint& pos) {
            int sourceIndex = -1;
            for (const auto& row : m_rows) {
                if (row.row == rowW) {
                    sourceIndex = row.sourceIndex;
                    break;
                }
            }
            if (sourceIndex < 0 || sourceIndex >= m_accounts.size())
                return;

            QMenu menu(target);
            QAction* editAct = menu.addAction(T("Edit", "تعديل"));
            QAction* selected = menu.exec(target->mapToGlobal(pos));
            if (selected == editAct)
                editAccountAt(sourceIndex);
        });
    };

    attachAccountContextMenu(rowW);
    attachAccountContextMenu(code);
    attachAccountContextMenu(nameBlock);
    attachAccountContextMenu(name);
    attachAccountContextMenu(type);
    attachAccountContextMenu(settlement);
    attachAccountContextMenu(currency);
    attachAccountContextMenu(amountLabel);
    attachAccountContextMenu(amount);

    m_rowsLayout->insertWidget(qMax(0, m_rowsLayout->count() - 1), rowW);
    rowW->show();
}

void Accountswidget::rebuildRows(const QList<AccountItem>& items)
{
    syncVisibleRowsToAccounts();
    clearRenderedRows();
    m_accounts.clear();
    m_currentPage = 0;

    QStringList seenCodes;
    QStringList seenNames;
    for (const auto& rawItem : items) {
        AccountItem item = rawItem;
        if (item.currency.trimmed().isEmpty())
            item.currency = (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"));

        const QString codeKey = item.code.trimmed().toCaseFolded();
        const QString nameKey = item.name.trimmed().toCaseFolded();
        if (!codeKey.isEmpty() && seenCodes.contains(codeKey))
            continue;
        if (!nameKey.isEmpty() && seenNames.contains(nameKey))
            continue;
        if (!codeKey.isEmpty()) seenCodes.append(codeKey);
        if (!nameKey.isEmpty()) seenNames.append(nameKey);
        m_accounts.append(item);
    }

    refreshCurrencyFilter();
    applyFilters();
}

QList<AccountItem> Accountswidget::currentItems() const
{
    QList<AccountItem> items = m_accounts;
    QStringList seenCodes;
    QStringList seenNames;
    QList<AccountItem> unique;

    for (const auto& row : m_rows) {
        if (row.sourceIndex >= 0 && row.sourceIndex < items.size()) {
            AccountItem item = row.item;
            if (row.amount)
                item.amount = row.amount->value();
            items[row.sourceIndex] = item;
        }
    }

    for (const auto& item : items) {
        const QString codeKey = item.code.trimmed().toCaseFolded();
        const QString nameKey = item.name.trimmed().toCaseFolded();

        if (codeKey.isEmpty() && nameKey.isEmpty())
            continue;
        if (!codeKey.isEmpty() && seenCodes.contains(codeKey))
            continue;
        if (!nameKey.isEmpty() && seenNames.contains(nameKey))
            continue;

        if (!codeKey.isEmpty()) seenCodes.append(codeKey);
        if (!nameKey.isEmpty()) seenNames.append(nameKey);
        unique.append(item);
    }
    return unique;
}

void Accountswidget::sortAndRebuild()
{
    syncVisibleRowsToAccounts();
    const bool asc = !m_sortCombo || m_sortCombo->currentIndex() == 0;
    std::sort(m_accounts.begin(), m_accounts.end(), [asc](const AccountItem& a, const AccountItem& b) {
        const QString left = !a.code.trimmed().isEmpty() ? a.code.trimmed() : a.name.trimmed();
        const QString right = !b.code.trimmed().isEmpty() ? b.code.trimmed() : b.name.trimmed();
        const int cmp = QString::localeAwareCompare(left, right);
        if (cmp == 0)
            return asc ? (QString::localeAwareCompare(a.name, b.name) < 0)
                       : (QString::localeAwareCompare(a.name, b.name) > 0);
        return asc ? (cmp < 0) : (cmp > 0);
    });
    m_currentPage = 0;
    applyFilters();
}

QList<int> Accountswidget::filteredAccountIndexes() const
{
    const QString search = m_searchEdit ? m_searchEdit->text().trimmed().toCaseFolded() : QString();
    const AccountTypeFilter typeFilter = currentTypeFilter();
    const int settlementFilter = m_settlementFilterCombo ? m_settlementFilterCombo->currentData().toInt() : -1;
    const QString currencyFilter = m_currencyFilterCombo ? m_currencyFilterCombo->currentData().toString().trimmed().toCaseFolded() : QString();

    QList<int> indexes;
    for (int i = 0; i < m_accounts.size(); ++i) {
        bool visible = true;
        const AccountItem& item = m_accounts[i];

        if (!accountMatchesFilter(item.type, typeFilter))
            visible = false;

        if (settlementFilter != -1 && item.allowSettlement != (settlementFilter == 1))
            visible = false;

        if (!currencyFilter.isEmpty() && item.currency.trimmed().toCaseFolded() != currencyFilter)
            visible = false;

        if (!search.isEmpty()) {
            const QString haystack = (item.code + QStringLiteral(" ") +
                                      item.name + QStringLiteral(" ") +
                                      accountTypeDisplayName(item.type) + QStringLiteral(" ") +
                                      settlementText(item.allowSettlement) + QStringLiteral(" ") +
                                      item.currency + QStringLiteral(" ") +
                                      QString::number(item.amount, 'f', 2)).toCaseFolded();
            if (!haystack.contains(search))
                visible = false;
        }

        if (visible)
            indexes.append(i);
    }
    return indexes;
}

void Accountswidget::renderCurrentPage()
{
    clearRenderedRows();

    const QList<int> indexes = filteredAccountIndexes();
    const int filteredCount = indexes.size();
    const int pages = totalPagesForCount(filteredCount);
    if (m_currentPage >= pages)
        m_currentPage = qMax(0, pages - 1);
    if (m_currentPage < 0)
        m_currentPage = 0;

    const int start = m_currentPage * kAccountsPerPage;
    const int end = qMin(start + kAccountsPerPage, filteredCount);
    for (int pos = start; pos < end; ++pos) {
        const int sourceIndex = indexes[pos];
        if (sourceIndex >= 0 && sourceIndex < m_accounts.size())
            addRenderedRow(m_accounts[sourceIndex], sourceIndex);
    }

    updatePaginationControls(filteredCount);

    if (!m_empty)
        return;

    if (m_accounts.isEmpty()) {
        m_empty->setText(tr_no_accounts_added_yet_b3f1b8());
        m_empty->show();
    } else if (filteredCount == 0) {
        m_empty->setText(T("No accounts match the selected filters.", "لا توجد حسابات تطابق الفلاتر المحددة."));
        m_empty->show();
    } else {
        m_empty->hide();
    }
}

void Accountswidget::applyFilters()
{
    syncVisibleRowsToAccounts();
    renderCurrentPage();
}

bool Accountswidget::openAccountDialog(AccountItem& item, bool editMode, int ignoreIndex)
{
    QDialog dialog(this);
    dialog.setWindowTitle(editMode ? T("Edit account", "تعديل الحساب")
                                   : T("Add account", "إضافة حساب"));
    dialog.setLayoutDirection(appLayoutDirection());
    dialog.setStyleSheet(dialogStyle());
    dialog.setMinimumWidth(560);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(14);

    auto* form = new QFormLayout;
    form->setLabelAlignment(appTextAlign());
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(10);

    auto* codeEdit = new QLineEdit(&dialog);
    auto* nameEdit = new QLineEdit(&dialog);
    auto* typeCombo = new NoWheelComboBox(&dialog);
    populateTypeCombo(typeCombo, false);

    auto* settlementSwitch = new QCheckBox(T("Allow settlement", "السماح بالتسوية"), &dialog);

    auto* currencyCombo = new NoWheelComboBox(&dialog);
    currencyCombo->setEditable(true);
    currencyCombo->addItem(QStringLiteral("USD"));
    currencyCombo->addItem(QStringLiteral("IQD"));

    auto* amountSpin = new NoWheelDoubleSpinBox(&dialog);
    amountSpin->setRange(-999999999999.99, 999999999999.99);
    amountSpin->setDecimals(2);
    amountSpin->setSingleStep(1000.0);
    amountSpin->setKeyboardTracking(false);
    amountSpin->setGroupSeparatorShown(true);
    amountSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    amountSpin->setAlignment(appTextAlign() | Qt::AlignVCenter);

    codeEdit->setPlaceholderText(T("Example: 6010", "مثال: 6010"));
    nameEdit->setPlaceholderText(T("Example: Office rent", "مثال: إيجار المكتب"));

    codeEdit->setText(item.code.trimmed());
    nameEdit->setText(item.name.trimmed());
    const int typeIndex = typeCombo->findData(int(item.type));
    typeCombo->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
    settlementSwitch->setChecked(item.allowSettlement);
    currencyCombo->setCurrentText(item.currency.trimmed().isEmpty()
        ? (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"))
        : item.currency.trimmed().toUpper());
    amountSpin->setValue(item.amount);

    form->addRow(T("Account code", "رمز الحساب"), codeEdit);
    form->addRow(T("Account name", "اسم الحساب"), nameEdit);
    form->addRow(T("Type", "النوع"), typeCombo);
    form->addRow(QString(), settlementSwitch);
    form->addRow(tr_currency_88f072(), currencyCombo);
    form->addRow(T("Amount", "المبلغ"), amountSpin);
    root->addLayout(form);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancelBtn = new QPushButton(tr_cancel_8d40ef(), &dialog);
    cancelBtn->setObjectName("cancelBtn");
    auto* confirmBtn = new QPushButton(editMode ? T("Save", "حفظ") : tr_add_a98dbf(), &dialog);
    buttons->addWidget(cancelBtn);
    buttons->addWidget(confirmBtn);
    root->addLayout(buttons);

    bool accepted = false;
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(confirmBtn, &QPushButton::clicked, &dialog, [&]() {
        AccountItem edited;
        edited.code = codeEdit->text().trimmed();
        edited.name = nameEdit->text().trimmed();
        edited.type = accountTypeFromCombo(typeCombo);
        edited.allowSettlement = settlementSwitch->isChecked();
        edited.currency = currencyCombo->currentText().trimmed().toUpper();
        edited.amount = amountSpin->value();

        if (edited.code.isEmpty() || edited.name.isEmpty()) {
            ThemeBox::warn(&dialog,
                T("Missing account details", "تفاصيل الحساب ناقصة"),
                T("Account code and account name are required.", "رمز الحساب واسم الحساب مطلوبان."));
            return;
        }

        if (hasDuplicateAccount(edited, ignoreIndex)) {
            ThemeBox::warn(&dialog,
                tr_duplicate_account_e404c5(),
                T("An account with the same code or name already exists.", "يوجد حساب بنفس الرمز أو الاسم بالفعل."));
            return;
        }

        item = edited;
        accepted = true;
        dialog.accept();
    });

    return dialog.exec() == QDialog::Accepted && accepted;
}

void Accountswidget::onAddAccount()
{
    AccountItem item;
    item.currency = (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"));
    item.type = AccountType::Payable;
    item.amount = 0.0;

    if (!openAccountDialog(item, false, -1))
        return;

    addRow(item);
    sortAndRebuild();
}

void Accountswidget::editAccountAt(int sourceIndex)
{
    syncVisibleRowsToAccounts();

    if (sourceIndex < 0 || sourceIndex >= m_accounts.size())
        return;

    AccountItem edited = m_accounts.at(sourceIndex);
    if (!openAccountDialog(edited, true, sourceIndex))
        return;

    m_accounts[sourceIndex] = edited;
    for (auto& row : m_rows) {
        if (row.sourceIndex == sourceIndex) {
            row.item = edited;
            if (row.amount) {
                const QSignalBlocker blocker(row.amount);
                row.amount->setValue(edited.amount);
            }
            break;
        }
    }
    refreshCurrencyFilter();
    sortAndRebuild();
}

void Accountswidget::onEditRow()
{
    auto* action = qobject_cast<QAction*>(sender());
    if (!action)
        return;
    editAccountAt(action->data().toInt());
}
void Accountswidget::onFilterChanged()
{
    m_currentPage = 0;
    applyFilters();
}

bool Accountswidget::showGraphSelectionForRequest(const ChartRequest& existing)
{
    syncVisibleRowsToAccounts();

    ChartRequest request = existing;
    if (!openGraphDialog(request, &existing))
        return false;

    emit graphRequested(request);
    return true;
}

void Accountswidget::onShowGraphs()
{
    syncVisibleRowsToAccounts();

    ChartRequest request;
    if (!openGraphDialog(request))
        return;

    emit graphRequested(request);
}

void Accountswidget::onRemoveRow()
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn)
        return;

    int sourceIndex = -1;
    for (const auto& row : m_rows) {
        if (row.removeBtn == btn) {
            sourceIndex = row.sourceIndex;
            break;
        }
    }

    if (sourceIndex >= 0 && sourceIndex < m_accounts.size())
        m_accounts.removeAt(sourceIndex);

    refreshCurrencyFilter();
    applyFilters();
}

AppData Accountswidget::collectData() const
{
    AppData d;
    d.accounts = currentItems();
    d.calculate();
    return d;
}

void Accountswidget::setData(const AppData& data)
{
    rebuildRows(data.accounts);
}

void Accountswidget::clearData()
{
    rebuildRows({});
}
