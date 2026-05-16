#include "Accountswidget.h"
#include "translations.h"
#include "themebox.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QGridLayout>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QWheelEvent>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QSpinBox>
#include <QToolButton>
#include <QMouseEvent>
#include <QLocale>
#include <QSizePolicy>

namespace {

class NoWheelComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;
protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); }
};

class NoWheelDoubleSpinBox : public QDoubleSpinBox
{
public:
    using QDoubleSpinBox::QDoubleSpinBox;
protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); }
};

static const char* kExpensesDark = R"(
QWidget#expensesRoot { background:#0d1020; }
QWidget#expensesHeader { background:#111526; border-bottom:1px solid #1a1f38; }
QWidget#expensesContainer { background:#0d1020; }
QScrollArea#expensesScroll { background:#0d1020; border:none; }
QFrame#expenseMonthCard { background:#141827; border:1px solid #252b4a; border-radius:10px; }
QWidget#expenseMonthHeader { background:#1a1f38; border-radius:9px 9px 0 0; }
QWidget#expenseMonthHeader:hover { background:#1e2445; }
QLabel#expenseMonthLabel { color:#c8d0ed; font-weight:700; background:transparent; }
QLabel#expenseChevron { color:#5a6490; background:transparent; }
QWidget#expenseMonthContent { background:#141827; border-radius:0 0 9px 9px; }
QLabel#expensesTitle { color:#c8d0ed; font-weight:900; font-size:18px; background:transparent; }
QLabel#expensesSubtitle { color:#8a94bd; background:transparent; }
QLabel#expensesLabel { color:#c8d0ed; background:transparent; font-weight:800; }
QLabel#expensesTableHeader { color:#9fbaff; background:transparent; font-weight:900; }
QWidget#expenseRow { background:#111526; border:1px solid #1f2742; border-radius:10px; }
QLabel#expenseAccountName { color:#e6ebff; background:transparent; font-weight:900; font-size:15px; }
QLabel#expenseFieldCaption { color:#8a94bd; background:transparent; font-weight:800; }
QComboBox, QDoubleSpinBox { background:#252d4a; color:#c8d0ed; border:1px solid #3a4268; border-radius:6px; padding:6px 8px; }
QComboBox:focus, QDoubleSpinBox:focus { border-color:#4f86f7; }
QScrollBar:vertical { background:#0d1020; width:8px; border-radius:4px; }
QScrollBar::handle:vertical { background:#2e3860; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#4f86f7; }
QListView { background:#1a1f38; color:#c8d0ed; selection-background-color:#4f86f7; selection-color:white; border:1px solid #252b52; }
QListView::item { padding:6px 8px; }
)";

static const char* kExpensesLight = R"(
QWidget#expensesRoot { background:#f4f6fb; }
QWidget#expensesHeader { background:#ffffff; border-bottom:1px solid #dde2f0; }
QWidget#expensesContainer { background:#f4f6fb; }
QScrollArea#expensesScroll { background:#f4f6fb; border:none; }
QFrame#expenseMonthCard { background:#ffffff; border:1px solid #dde2f0; border-radius:10px; }
QWidget#expenseMonthHeader { background:#f4f6fb; border-radius:9px 9px 0 0; }
QWidget#expenseMonthHeader:hover { background:#eef0fa; }
QLabel#expenseMonthLabel { color:#1e2340; font-weight:700; background:transparent; }
QLabel#expenseChevron { color:#8892b8; background:transparent; }
QWidget#expenseMonthContent { background:#ffffff; border-radius:0 0 9px 9px; }
QLabel#expensesTitle { color:#1e2340; font-weight:900; font-size:18px; background:transparent; }
QLabel#expensesSubtitle { color:#6b7280; background:transparent; }
QLabel#expensesLabel { color:#1e2340; background:transparent; font-weight:800; }
QLabel#expensesTableHeader { color:#1d4ed8; background:transparent; font-weight:900; }
QWidget#expenseRow { background:#ffffff; border:1px solid #dde2f0; border-radius:10px; }
QLabel#expenseAccountName { color:#1e2340; background:transparent; font-weight:900; font-size:15px; }
QLabel#expenseFieldCaption { color:#6b7280; background:transparent; font-weight:800; }
QComboBox, QDoubleSpinBox { background:#ffffff; color:#1e2340; border:1px solid #cfd7ea; border-radius:6px; padding:6px 8px; }
QComboBox:focus, QDoubleSpinBox:focus { border-color:#4f86f7; }
QScrollBar:vertical { background:#f4f6fb; width:8px; border-radius:4px; }
QScrollBar::handle:vertical { background:#c8d0ed; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#4f86f7; }
QListView { background:#ffffff; color:#1e2340; selection-background-color:#eef0fa; selection-color:#1e2340; border:1px solid #dde2f0; }
QListView::item { padding:6px 8px; }
)";

static void styleComboPopup(QComboBox* combo)
{
    if (!combo || !combo->view())
        return;
    combo->view()->setAttribute(Qt::WA_StyledBackground, true);
}

static Qt::Alignment expenseCellTextAlignment()
{
    return (isArabic() ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter;
}

static void applyExpenseLabelDirection(QLabel* label)
{
    if (!label)
        return;
    label->setLayoutDirection(appLayoutDirection());
    label->setAlignment(expenseCellTextAlignment());
}

static void applyExpenseSpinDirection(QDoubleSpinBox* spin)
{
    if (!spin)
        return;
    spin->setLayoutDirection(appLayoutDirection());
    spin->setAlignment(expenseCellTextAlignment());
}

static void applyExpenseComboDirection(QComboBox* combo)
{
    if (!combo)
        return;
    combo->setLayoutDirection(appLayoutDirection());
    for (int i = 0; i < combo->count(); ++i)
        combo->setItemData(i, int(expenseCellTextAlignment()), Qt::TextAlignmentRole);
    if (combo->lineEdit()) {
        combo->lineEdit()->setLayoutDirection(appLayoutDirection());
        combo->lineEdit()->setAlignment(expenseCellTextAlignment());
    }
}


class ExpenseStayOpenMenu : public QMenu
{
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

static QList<int> expenseMonthsWithData(const AppData& data)
{
    QList<int> out;
    for (int month = 0; month < 12; ++month) {
        const QList<AccountItem> list = normalizedFixedExpenseAccountsForMonth(data.monthlyAccounts[month]);
        bool hasData = false;
        for (const AccountItem& item : list) {
            if (item.amount != 0.0) {
                hasData = true;
                break;
            }
        }
        if (hasData)
            out << month;
    }
    return out;
}

static QString expenseMonthSummaryText(const QList<int>& months)
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

class ExpenseGraphSelectionDialog : public QDialog
{
public:
    ExpenseGraphSelectionDialog(ChartKind kind, const AppData& data, QWidget* parent = nullptr, const ChartRequest* existing = nullptr)
        : QDialog(parent), m_kind(kind)
    {
        setWindowTitle(tr_show_graphs_26cf20());
        setModal(true);
        setMinimumWidth(520);
        setLayoutDirection(appLayoutDirection());
        setStyleSheet(g_lightMode
            ? QStringLiteral("QDialog{background:#f4f6fb;} QLabel{color:#1e2340;background:transparent;} QToolButton,QPushButton,QComboBox,QSpinBox{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:7px;padding:7px 12px;font-weight:700;} QToolButton:hover,QPushButton:hover,QComboBox:hover,QSpinBox:hover{background:#eef0fa;} QMenu{background:#ffffff;color:#1e2340;border:1px solid #dde2f0;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#eef0fa;} QCheckBox{color:#1e2340;background:transparent;font-weight:700;} QCheckBox::indicator{width:17px;height:17px;border:1px solid #8fa1c2;border-radius:4px;background:#ffffff;} QCheckBox::indicator:checked{background:#4f86f7;border:1px solid #356ed6;} QCheckBox::indicator:disabled{background:#eef0fa;border:1px solid #cfd7ea;}")
            : QStringLiteral("QDialog{background:#12152a;} QLabel{color:#e6ebff;background:transparent;} QToolButton,QPushButton,QComboBox,QSpinBox{background:#1a1f38;color:#e7ecff;border:1px solid #343c63;border-radius:7px;padding:7px 12px;font-weight:700;} QToolButton:hover,QPushButton:hover,QComboBox:hover,QSpinBox:hover{background:#1e2445;} QMenu{background:#1a1f38;color:#e7ecff;border:1px solid #343c63;} QMenu::item{padding:6px 18px;} QMenu::item:selected{background:#4f86f7;color:#ffffff;} QCheckBox{color:#e6ebff;background:transparent;font-weight:700;} QCheckBox::indicator{width:17px;height:17px;border:1px solid #59648c;border-radius:4px;background:#12152a;} QCheckBox::indicator:checked{background:#4f86f7;border:1px solid #7ba7ff;} QCheckBox::indicator:disabled{background:#1a1f38;border:1px solid #343c63;}")
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
        m_chartTypeCombo->addItem(tr_pie_chart_9d4e04(), int(ChartKind::Pie));
        m_chartTypeCombo->addItem(tr_bar_chart_a5f324(), int(ChartKind::RankedBar));
        m_chartTypeCombo->addItem(T("Horizontal bar", "شريط أفقي"), int(ChartKind::HorizontalBar));
        m_chartTypeCombo->addItem(tr_line_chart_932796(), int(ChartKind::MetricLine));
        m_chartTypeCombo->addItem(tr_candle_chart_f7a9c2(), int(ChartKind::Candle));
        int typeIndex = m_chartTypeCombo->findData(int(m_kind));
        if (typeIndex < 0)
            typeIndex = m_chartTypeCombo->findData(int(ChartKind::RankedBar));
        m_chartTypeCombo->setCurrentIndex(typeIndex < 0 ? 0 : typeIndex);
        m_kind = static_cast<ChartKind>(m_chartTypeCombo->currentData().toInt());
        connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            if (!m_chartTypeCombo) return;
            m_kind = static_cast<ChartKind>(m_chartTypeCombo->currentData().toInt());
            updateSummaryAvailability();
        });
        root->addWidget(m_chartTypeCombo);

        auto* filterLabel = new QLabel(tr_group_by_2bda9d());
        filterLabel->setStyleSheet(QStringLiteral("font-weight:800;"));
        root->addWidget(filterLabel);

        m_accountFilterCombo = new QComboBox(this);
        styleComboPopup(m_accountFilterCombo);
        m_accountFilterCombo->addItem(tr_all_b4d286(), int(AccountTypeFilter::All));
        m_accountFilterCombo->addItem(tr_account_receivable_59bf34(), int(AccountTypeFilter::Receivable));
        m_accountFilterCombo->addItem(tr_account_payable_003206(), int(AccountTypeFilter::Payable));
        const int selectedFilter = existing ? int(existing->accountFilter) : int(AccountTypeFilter::All);
        int filterIndex = m_accountFilterCombo->findData(selectedFilter);
        if (filterIndex < 0)
            filterIndex = m_accountFilterCombo->findData(int(AccountTypeFilter::All));
        m_accountFilterCombo->setCurrentIndex(filterIndex < 0 ? 0 : filterIndex);
        root->addWidget(m_accountFilterCombo);

        auto* topLabel = new QLabel(T("Top accounts", "أعلى الحسابات"));
        topLabel->setStyleSheet(QStringLiteral("font-weight:800;"));
        root->addWidget(topLabel);

        m_topCount = new QSpinBox(this);
        m_topCount->setRange(1, 999);
        m_topCount->setValue(existing && existing->topAccountCount > 0 ? existing->topAccountCount : 10);
        root->addWidget(m_topCount);

        auto* monthsLabel = new QLabel(tr_choose_months_ff1808());
        monthsLabel->setStyleSheet(QStringLiteral("font-weight:800;"));
        root->addWidget(monthsLabel);

        m_monthsBtn = new QToolButton(this);
        m_monthsBtn->setPopupMode(QToolButton::InstantPopup);
        m_monthsBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        m_monthsBtn->setArrowType(Qt::DownArrow);
        auto* monthsMenu = new ExpenseStayOpenMenu(m_monthsBtn);
        QAction* selectAllMonths = monthsMenu->addAction(tr_select_all_7812c3());
        QAction* deselectAllMonths = monthsMenu->addAction(tr_deselect_all_474bc1());
        monthsMenu->addSeparator();
        const QList<int> dataMonths = expenseMonthsWithData(data);
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
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        root->addWidget(buttons);

        updateMonthButton();
    }

    ChartKind kind() const { return m_kind; }

    AccountTypeFilter accountFilter() const
    {
        if (!m_accountFilterCombo || m_accountFilterCombo->currentIndex() < 0)
            return AccountTypeFilter::All;
        return static_cast<AccountTypeFilter>(m_accountFilterCombo->currentData().toInt());
    }

    int topAccountCount() const
    {
        return m_topCount ? m_topCount->value() : 10;
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

private:
    void updateSummaryAvailability()
    {
        if (!m_summaryCheck)
            return;
        const bool isPie = (m_kind == ChartKind::Pie || m_kind == ChartKind::ComparePie);
        m_summaryCheck->setEnabled(!isPie);
        if (isPie)
            m_summaryCheck->setChecked(false);
    }

    void updateMonthButton()
    {
        QList<int> months;
        for (QAction* action : m_monthActions) {
            if (action && action->isChecked())
                months << action->data().toInt();
        }
        if (m_monthsBtn)
            m_monthsBtn->setText(tr_months_1_b69e08().arg(expenseMonthSummaryText(months)));
    }

    ChartKind m_kind;
    QComboBox* m_chartTypeCombo{};
    QComboBox* m_accountFilterCombo{};
    QSpinBox* m_topCount{};
    QToolButton* m_monthsBtn{};
    QCheckBox* m_summaryCheck{};
    QVector<QAction*> m_monthActions;
};

} // namespace

Accountswidget::Accountswidget(QWidget* parent)
    : QWidget(parent)
{
    initializeMonthData();
    buildUi();
    retranslate();
    applyTheme();
    renderCurrentMonth();
}

void Accountswidget::buildUi()
{
    setObjectName("expensesRoot");
    setLayoutDirection(appLayoutDirection());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    header->setObjectName("expensesHeader");
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(20, 16, 20, 14);
    headerLayout->setSpacing(10);

    m_titleRow = new QHBoxLayout;
    m_titleRow->setContentsMargins(0, 0, 0, 0);
    m_titleRow->setSpacing(10);
    m_titleRow->setDirection(isArabic() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);

    m_title = new QLabel(header);
    m_title->setObjectName("expensesTitle");
    m_graphBtn = new QToolButton(header);
    m_graphBtn->setObjectName("showGraphsBtn");
    m_graphBtn->setCursor(Qt::PointingHandCursor);
    m_graphBtn->setFixedHeight(34);

    m_titleRow->addWidget(m_title);
    m_titleRow->addStretch();
    m_titleRow->addWidget(m_graphBtn, 0, isArabic() ? Qt::AlignLeft : Qt::AlignRight);

    m_subtitle = new QLabel(header);
    m_subtitle->setObjectName("expensesSubtitle");
    m_subtitle->setWordWrap(true);

    headerLayout->addLayout(m_titleRow);
    headerLayout->addWidget(m_subtitle);
    connect(m_graphBtn, &QAbstractButton::clicked, this, &Accountswidget::onShowGraphs);

    auto* selectorRow = new QHBoxLayout;
    selectorRow->setDirection(isArabic() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);
    selectorRow->setSpacing(10);

    m_groupLabel = new QLabel(header);
    m_groupLabel->setObjectName("expensesLabel");
    m_groupCombo = new NoWheelComboBox(header);
    m_groupCombo->setMinimumWidth(220);
    styleComboPopup(m_groupCombo);
    m_groupCombo->addItem(tr_all_b4d286(), int(AccountTypeFilter::All));
    m_groupCombo->addItem(tr_account_receivable_59bf34(), int(AccountTypeFilter::Receivable));
    m_groupCombo->addItem(tr_account_payable_003206(), int(AccountTypeFilter::Payable));

    selectorRow->addWidget(m_groupLabel);
    selectorRow->addWidget(m_groupCombo);
    selectorRow->addStretch();
    headerLayout->addLayout(selectorRow);
    root->addWidget(header);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("expensesScroll");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget;
    m_container->setObjectName("expensesContainer");
    m_container->setAttribute(Qt::WA_StyledBackground, true);
    m_cardsLayout = new QVBoxLayout(m_container);
    m_cardsLayout->setContentsMargins(20, 16, 20, 24);
    m_cardsLayout->setSpacing(10);

    for (int month = 0; month < 12; ++month) {
        MonthWidgets card;
        card.monthIndex = month;
        card.expanded = (month == 0);
        card.card = new QFrame(m_container);
        card.card->setObjectName("expenseMonthCard");
        card.card->setAttribute(Qt::WA_StyledBackground, true);

        auto* cardLayout = new QVBoxLayout(card.card);
        cardLayout->setContentsMargins(0, 0, 0, 0);
        cardLayout->setSpacing(0);

        card.header = new QWidget(card.card);
        card.header->setObjectName("expenseMonthHeader");
        card.header->setAttribute(Qt::WA_StyledBackground, true);
        card.header->setCursor(Qt::PointingHandCursor);
        card.header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        card.header->setFixedHeight(52);
        card.header->setProperty("monthIndex", month);
        card.header->installEventFilter(this);

        auto* headerLayout = new QHBoxLayout(card.header);
        headerLayout->setContentsMargins(18, 0, 18, 0);
        headerLayout->setSpacing(10);

        card.monthLabel = new QLabel(card.header);
        card.monthLabel->setObjectName("expenseMonthLabel");
        card.monthLabel->setAlignment(expenseCellTextAlignment());
        card.monthLabel->setLayoutDirection(appLayoutDirection());

        card.chevron = new QLabel(card.header);
        card.chevron->setObjectName("expenseChevron");
        card.chevron->setAlignment(Qt::AlignCenter);

        headerLayout->addWidget(card.monthLabel);
        headerLayout->addStretch();
        headerLayout->addWidget(card.chevron);
        cardLayout->addWidget(card.header);

        card.content = new QWidget(card.card);
        card.content->setObjectName("expenseMonthContent");
        card.content->setAttribute(Qt::WA_StyledBackground, true);
        auto* contentLayout = new QVBoxLayout(card.content);
        contentLayout->setContentsMargins(14, 12, 14, 14);
        contentLayout->setSpacing(9);

        auto* tableHeader = new QWidget(card.content);
        tableHeader->setObjectName("expenseTableHeaderWidget");
        tableHeader->setLayoutDirection(Qt::LeftToRight);
        auto* headerGrid = new QGridLayout(tableHeader);
        headerGrid->setContentsMargins(0, 0, 0, 4);
        headerGrid->setHorizontalSpacing(12);
        headerGrid->setVerticalSpacing(0);
        headerGrid->setOriginCorner(Qt::TopLeftCorner);

        card.accountHeader = new QLabel(tableHeader);
        card.accountHeader->setObjectName("expensesTableHeader");
        card.amountHeader = new QLabel(tableHeader);
        card.amountHeader->setObjectName("expensesTableHeader");
        card.typeHeader = new QLabel(tableHeader);
        card.typeHeader->setObjectName("expensesTableHeader");

        const int accountCol = isArabic() ? 2 : 0;
        const int amountCol = 1;
        const int typeCol = isArabic() ? 0 : 2;
        headerGrid->addWidget(card.accountHeader, 0, accountCol, expenseCellTextAlignment());
        headerGrid->addWidget(card.amountHeader, 0, amountCol, expenseCellTextAlignment());
        headerGrid->addWidget(card.typeHeader, 0, typeCol, expenseCellTextAlignment());
        headerGrid->setColumnStretch(accountCol, 2);
        headerGrid->setColumnStretch(amountCol, 1);
        headerGrid->setColumnStretch(typeCol, 1);
        contentLayout->addWidget(tableHeader);

        card.rowsLayout = new QVBoxLayout;
        card.rowsLayout->setContentsMargins(0, 0, 0, 0);
        card.rowsLayout->setSpacing(8);
        contentLayout->addLayout(card.rowsLayout);

        cardLayout->addWidget(card.content);
        m_cardsLayout->addWidget(card.card);
        m_monthCards.append(card);

    }
    m_cardsLayout->addStretch();
    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll, 1);

    connect(m_groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_loadingRows)
            return;
        syncRowsToCurrentMonth();
        renderCurrentMonth();
    });
}

void Accountswidget::initializeMonthData()
{
    for (auto& month : m_monthlyAccounts)
        month = defaultFixedExpenseAccounts();
}

QString Accountswidget::accountTypeLabel(AccountType type) const
{
    return type == AccountType::Receivable ? tr_account_receivable_59bf34()
                                           : tr_account_payable_003206();
}

void Accountswidget::setComboToAccountType(QComboBox* combo, AccountType type) const
{
    if (!combo)
        return;
    const int target = (type == AccountType::Receivable) ? int(AccountType::Receivable) : int(AccountType::Payable);
    const int idx = combo->findData(target);
    combo->setCurrentIndex(idx >= 0 ? idx : 0);
}

AccountType Accountswidget::accountTypeFromCombo(const QComboBox* combo) const
{
    if (!combo)
        return AccountType::Payable;
    return combo->currentData().toInt() == int(AccountType::Receivable)
        ? AccountType::Receivable
        : AccountType::Payable;
}

AccountTypeFilter Accountswidget::currentGroupFilter() const
{
    if (!m_groupCombo)
        return AccountTypeFilter::All;
    const int value = m_groupCombo->currentData().toInt();
    if (value == int(AccountTypeFilter::Receivable))
        return AccountTypeFilter::Receivable;
    if (value == int(AccountTypeFilter::Payable))
        return AccountTypeFilter::Payable;
    return AccountTypeFilter::All;
}

QString Accountswidget::nextCustomAccountCode() const
{
    int maxId = 0;
    for (const auto& month : m_monthlyAccounts) {
        for (const auto& item : month) {
            const QString code = item.code.trimmed().toUpper();
            if (!code.startsWith(QStringLiteral("CU")))
                continue;
            bool ok = false;
            const int id = code.mid(2).toInt(&ok);
            if (ok)
                maxId = qMax(maxId, id);
        }
    }
    return QStringLiteral("CU%1").arg(maxId + 1, 3, 10, QChar('0'));
}

void Accountswidget::deleteAccountAtIndex(int monthIndex, int accountIndex)
{
    syncRowsToCurrentMonth();
    if (monthIndex < 0 || monthIndex >= 12)
        return;
    const QList<AccountItem> current = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[monthIndex]);
    if (accountIndex < 0 || accountIndex >= current.size())
        return;

    const AccountItem target = current[accountIndex];
    const QString key = normalizedAccountKey(target);
    if (ThemeBox::confirm(this, tr_delete_account_title_b23407(), tr_delete_account_warning_f0c88a()) != QMessageBox::Yes)
        return;

    for (auto& month : m_monthlyAccounts) {
        QList<AccountItem> normalized = normalizedFixedExpenseAccountsForMonth(month);
        for (int i = normalized.size() - 1; i >= 0; --i) {
            if (normalizedAccountKey(normalized[i]) == key)
                normalized.removeAt(i);
        }
        month = normalized;
    }
    renderCurrentMonth();
    emit dataChanged();
}

void Accountswidget::addAccount()
{
    syncRowsToCurrentMonth();
    bool ok = false;
    QInputDialog dialog(this);
    dialog.setWindowTitle(tr_new_expense_account_title_d82b71());
    dialog.setLabelText(tr_new_expense_account_prompt_9747ab());
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setTextEchoMode(QLineEdit::Normal);
    dialog.setLayoutDirection(appLayoutDirection());
    dialog.setStyleSheet(g_lightMode
        ? QStringLiteral("QInputDialog{background:#ffffff;color:#1e2340;} QLabel{color:#1e2340;background:transparent;font-weight:700;} QLineEdit{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:6px 8px;} QPushButton{background:#eef0fa;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:6px 14px;} QPushButton:hover{background:#dfe7fb;}")
        : QStringLiteral("QInputDialog{background:#111526;color:#e7ecff;} QLabel{color:#e7ecff;background:transparent;font-weight:700;} QLineEdit{background:#252d4a;color:#e7ecff;border:1px solid #3a4268;border-radius:6px;padding:6px 8px;} QPushButton{background:#252d4a;color:#e7ecff;border:1px solid #3a4268;border-radius:6px;padding:6px 14px;} QPushButton:hover{background:#33406a;}"));
    ok = (dialog.exec() == QDialog::Accepted);
    const QString name = dialog.textValue().trimmed();
    if (!ok || name.isEmpty())
        return;

    const QString code = nextCustomAccountCode();
    const AccountType newType = currentGroupFilter() == AccountTypeFilter::Receivable ? AccountType::Receivable : AccountType::Payable;
    for (auto& month : m_monthlyAccounts) {
        QList<AccountItem> normalized = normalizedFixedExpenseAccountsForMonth(month);
        AccountItem item;
        item.code = code;
        item.name = name;
        item.currency = (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"));
        item.type = newType;
        item.amount = 0.0;
        normalized.append(item);
        month = normalized;
    }
    renderCurrentMonth();
    emit dataChanged();
}

void Accountswidget::syncRowsToCurrentMonth() const
{
    for (const auto& card : m_monthCards) {
        const int monthIndex = card.monthIndex;
        if (monthIndex < 0 || monthIndex >= 12)
            continue;

        QList<AccountItem> month = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[monthIndex]);
        for (const auto& row : card.rows) {
            const int idx = row.accountIndex;
            if (idx < 0 || idx >= month.size())
                continue;
            if (row.amount)
                month[idx].amount = row.amount->value();
            if (row.type)
                month[idx].type = accountTypeFromCombo(row.type);
            if (fixedExpenseAccountIndexFromItem(month[idx]) >= 0) {
                month[idx].name = fixedExpenseAccountNames().value(fixedExpenseAccountIndexFromItem(month[idx]));
                month[idx].code = fixedExpenseAccountCode(fixedExpenseAccountIndexFromItem(month[idx]));
            }
            month[idx].currency = (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"));
        }
        m_monthlyAccounts[monthIndex] = month;
    }
}

void Accountswidget::clearRows()
{
    for (auto& card : m_monthCards) {
        while (!card.rows.isEmpty()) {
            RowWidgets row = card.rows.takeLast();
            if (row.row) {
                if (card.rowsLayout)
                    card.rowsLayout->removeWidget(row.row);
                delete row.row;
            }
        }
    }
}

void Accountswidget::renderCurrentMonth()
{
    clearRows();
    m_loadingRows = true;
    const AccountTypeFilter filter = currentGroupFilter();

    for (auto& card : m_monthCards) {
        const int month = qBound(0, card.monthIndex, 11);
        QList<AccountItem> items = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[month]);
        m_monthlyAccounts[month] = items;
        updateMonthCardText(card);

        if (!card.rowsLayout)
            continue;

        for (int i = 0; i < items.size(); ++i) {
            const AccountItem& item = items[i];
            if (!accountMatchesFilter(item.type, filter))
                continue;

            auto* rowW = new QWidget(card.content ? card.content : m_container);
            rowW->setObjectName("expenseRow");
            rowW->setLayoutDirection(Qt::LeftToRight);
            rowW->setContextMenuPolicy(Qt::CustomContextMenu);

            auto* rowLayout = new QGridLayout(rowW);
            rowLayout->setContentsMargins(14, 10, 14, 10);
            rowLayout->setHorizontalSpacing(12);
            rowLayout->setVerticalSpacing(0);
            rowLayout->setOriginCorner(Qt::TopLeftCorner);

            auto* accountLabel = new QLabel(expenseAccountDisplayName(item), rowW);
            accountLabel->setObjectName("expenseAccountName");
            applyExpenseLabelDirection(accountLabel);
            accountLabel->setWordWrap(true);

            auto* amountSpin = new NoWheelDoubleSpinBox(rowW);
            amountSpin->setRange(-999999999999.99, 999999999999.99);
            amountSpin->setDecimals(currencyDecimals());
            amountSpin->setSingleStep(g_currency == AppCurrency::IQD ? 1000.0 : 100.0);
            amountSpin->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
            amountSpin->setValue(item.amount);
            amountSpin->setKeyboardTracking(false);
            amountSpin->setGroupSeparatorShown(true);
            amountSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
            amountSpin->setPrefix(currencyPrefix());
            amountSpin->setSuffix(currencySuffix());
            applyExpenseSpinDirection(amountSpin);

            auto* typeCombo = new NoWheelComboBox(rowW);
            typeCombo->addItem(accountTypeLabel(AccountType::Receivable), int(AccountType::Receivable));
            typeCombo->addItem(accountTypeLabel(AccountType::Payable), int(AccountType::Payable));
            setComboToAccountType(typeCombo, item.type);
            typeCombo->setMinimumWidth(180);
            styleComboPopup(typeCombo);
            applyExpenseComboDirection(typeCombo);

            const int accountCol = isArabic() ? 2 : 0;
            const int amountCol = 1;
            const int typeCol = isArabic() ? 0 : 2;
            rowLayout->addWidget(accountLabel, 0, accountCol, expenseCellTextAlignment());
            rowLayout->addWidget(amountSpin, 0, amountCol);
            rowLayout->addWidget(typeCombo, 0, typeCol);
            rowLayout->setColumnStretch(accountCol, 2);
            rowLayout->setColumnStretch(amountCol, 1);
            rowLayout->setColumnStretch(typeCol, 1);

            RowWidgets widgets;
            widgets.row = rowW;
            widgets.accountLabel = accountLabel;
            widgets.amountCaption = nullptr;
            widgets.typeCaption = nullptr;
            widgets.amount = amountSpin;
            widgets.type = typeCombo;
            widgets.monthIndex = month;
            widgets.accountIndex = i;
            card.rows.append(widgets);

            connect(rowW, &QWidget::customContextMenuRequested, this, [this, month, i, rowW](const QPoint& pos) {
                QMenu menu;
                menu.setStyleSheet(g_lightMode
                    ? QStringLiteral("QMenu{background:#ffffff;color:#1e2340;border:1px solid #d9e0ef;padding:4px;}QMenu::item{padding:7px 22px;}QMenu::item:selected{background:#eef0fa;color:#1e2340;}")
                    : QStringLiteral("QMenu{background:#1a1f38;color:#e7ecff;border:1px solid #343c63;padding:4px;}QMenu::item{padding:7px 22px;}QMenu::item:selected{background:#4f86f7;color:#ffffff;}"));
                QAction* del = menu.addAction(tr_delete_account_6dd013());
                QAction* chosen = menu.exec(rowW->mapToGlobal(pos));
                if (chosen == del)
                    QTimer::singleShot(0, this, [this, month, i]() { deleteAccountAtIndex(month, i); });
            });

            connect(amountSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, month, i](double value) {
                if (m_loadingRows || month < 0 || month >= 12)
                    return;
                QList<AccountItem> monthData = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[month]);
                if (i >= 0 && i < monthData.size()) {
                    monthData[i].amount = value;
                    monthData[i].currency = (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"));
                    m_monthlyAccounts[month] = monthData;
                    emit dataChanged();
                }
            });

            connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, month, i](int) {
                if (m_loadingRows || month < 0 || month >= 12)
                    return;

                QList<AccountItem> currentMonth = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[month]);
                if (i < 0 || i >= currentMonth.size())
                    return;

                const QString key = normalizedAccountKey(currentMonth[i]);
                AccountType newType = AccountType::Payable;
                for (const auto& cardForSearch : m_monthCards) {
                    for (const auto& r : cardForSearch.rows) {
                        if (r.monthIndex == month && r.accountIndex == i && r.type) {
                            newType = accountTypeFromCombo(r.type);
                            break;
                        }
                    }
                }

                for (auto& monthAccounts : m_monthlyAccounts) {
                    QList<AccountItem> normalized = normalizedFixedExpenseAccountsForMonth(monthAccounts);
                    for (auto& account : normalized) {
                        if (normalizedAccountKey(account) == key)
                            account.type = newType;
                    }
                    monthAccounts = normalized;
                }

                emit dataChanged();
                if (!accountMatchesFilter(newType, currentGroupFilter()))
                    renderCurrentMonth();
            });

            card.rowsLayout->addWidget(rowW);
        }
    }

    m_loadingRows = false;
}

void Accountswidget::updateRowTexts()
{
    for (auto& card : m_monthCards) {
        updateMonthCardText(card);
        const int month = qBound(0, card.monthIndex, 11);
        const QList<AccountItem> items = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[month]);
        for (auto& row : card.rows) {
            if (row.accountLabel && row.accountIndex >= 0 && row.accountIndex < items.size()) {
                row.accountLabel->setText(expenseAccountDisplayName(items.value(row.accountIndex)));
                applyExpenseLabelDirection(row.accountLabel);
            }
            if (row.amountCaption)
                row.amountCaption->setText(tr_expense_amount_field_93a771());
            if (row.typeCaption)
                row.typeCaption->setText(tr_expense_account_type_field_a870c9());
            if (row.amount) {
                row.amount->setPrefix(currencyPrefix());
                row.amount->setSuffix(currencySuffix());
                row.amount->setDecimals(currencyDecimals());
                row.amount->setSingleStep(g_currency == AppCurrency::IQD ? 1000.0 : 100.0);
                applyExpenseSpinDirection(row.amount);
            }
            if (row.type) {
                const AccountType selected = accountTypeFromCombo(row.type);
                const QSignalBlocker blocker(row.type);
                row.type->clear();
                row.type->addItem(accountTypeLabel(AccountType::Receivable), int(AccountType::Receivable));
                row.type->addItem(accountTypeLabel(AccountType::Payable), int(AccountType::Payable));
                setComboToAccountType(row.type, selected);
                applyExpenseComboDirection(row.type);
            }
        }
    }
}

void Accountswidget::updateMonthCardText(MonthWidgets& card)
{
    const QStringList months = monthNames();
    const QString monthName = (card.monthIndex >= 0 && card.monthIndex < months.size()) ? months.value(card.monthIndex) : QString();
    const QString arrow = card.expanded ? QStringLiteral("▲") : QStringLiteral("▼");
    if (card.header) {
        card.header->setLayoutDirection(Qt::LeftToRight);
        if (auto* layout = dynamic_cast<QHBoxLayout*>(card.header->layout())) {
            while (QLayoutItem* item = layout->takeAt(0))
                delete item;
            layout->setDirection(QBoxLayout::LeftToRight);
            if (isArabic()) {
                if (card.chevron)
                    layout->addWidget(card.chevron);
                layout->addStretch();
                if (card.monthLabel)
                    layout->addWidget(card.monthLabel);
            } else {
                if (card.monthLabel)
                    layout->addWidget(card.monthLabel);
                layout->addStretch();
                if (card.chevron)
                    layout->addWidget(card.chevron);
            }
        }
    }
    if (card.monthLabel) {
        card.monthLabel->setText(monthName);
        card.monthLabel->setLayoutDirection(appLayoutDirection());
        card.monthLabel->setAlignment(isArabic() ? (Qt::AlignRight | Qt::AlignVCenter) : (Qt::AlignLeft | Qt::AlignVCenter));
    }
    if (card.chevron)
        card.chevron->setText(arrow);
    if (card.content)
        card.content->setVisible(card.expanded);

    auto repositionHeader = [](QLabel* accountHeader, QLabel* amountHeader, QLabel* typeHeader) {
        if (!accountHeader || !amountHeader || !typeHeader)
            return;
        auto* tableHeader = accountHeader->parentWidget();
        if (!tableHeader)
            return;
        auto* grid = dynamic_cast<QGridLayout*>(tableHeader->layout());
        if (!grid)
            return;
        grid->setOriginCorner(Qt::TopLeftCorner);
        tableHeader->setLayoutDirection(Qt::LeftToRight);
        grid->removeWidget(accountHeader);
        grid->removeWidget(amountHeader);
        grid->removeWidget(typeHeader);
        const int accountCol = isArabic() ? 2 : 0;
        const int amountCol = 1;
        const int typeCol = isArabic() ? 0 : 2;
        grid->addWidget(accountHeader, 0, accountCol, expenseCellTextAlignment());
        grid->addWidget(amountHeader, 0, amountCol, expenseCellTextAlignment());
        grid->addWidget(typeHeader, 0, typeCol, expenseCellTextAlignment());
        grid->setColumnStretch(accountCol, 2);
        grid->setColumnStretch(amountCol, 1);
        grid->setColumnStretch(typeCol, 1);
    };

    repositionHeader(card.accountHeader, card.amountHeader, card.typeHeader);
    if (card.accountHeader) {
        card.accountHeader->setText(tr_fixed_expense_account_header_a13bcd());
        applyExpenseLabelDirection(card.accountHeader);
    }
    if (card.amountHeader) {
        card.amountHeader->setText(tr_expense_amount_field_93a771());
        applyExpenseLabelDirection(card.amountHeader);
    }
    if (card.typeHeader) {
        card.typeHeader->setText(tr_expense_account_type_field_a870c9());
        applyExpenseLabelDirection(card.typeHeader);
    }
}


bool Accountswidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event && event->type() == QEvent::MouseButtonRelease) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) {
            for (int i = 0; i < m_monthCards.size(); ++i) {
                if (obj == m_monthCards[i].header) {
                    setMonthExpanded(i, !m_monthCards[i].expanded);
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void Accountswidget::setMonthExpanded(int month, bool expanded)
{
    if (month < 0 || month >= m_monthCards.size())
        return;
    auto& card = m_monthCards[month];
    card.expanded = expanded;
    if (card.content)
        card.content->setVisible(expanded);
    updateMonthCardText(card);
}

void Accountswidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kExpensesLight : kExpensesDark);
    if (m_container)
        m_container->setStyleSheet(g_lightMode ? QStringLiteral("background:#f4f6fb;") : QStringLiteral("background:#0d1020;"));
    if (m_graphBtn) {
        m_graphBtn->setStyleSheet(g_lightMode
            ? QStringLiteral("QToolButton#showGraphsBtn{background:#4f86f7;color:white;border:none;border-radius:7px;padding:8px 14px;font-weight:700;}QToolButton#showGraphsBtn:hover{background:#5e91f8;}QToolButton#showGraphsBtn:pressed{background:#3a6fe0;}")
            : QStringLiteral("QToolButton#showGraphsBtn{background:#4f86f7;color:white;border:none;border-radius:7px;padding:8px 14px;font-weight:700;}QToolButton#showGraphsBtn:hover{background:#5e91f8;}QToolButton#showGraphsBtn:pressed{background:#3a6fe0;}"));
    }
}

void Accountswidget::retranslate()
{
    const bool wasLoading = m_loadingRows;
    m_loadingRows = true;
    syncRowsToCurrentMonth();
    setLayoutDirection(appLayoutDirection());

    if (m_title) {
        m_title->setText(tr_expenses_13597e());
        m_title->setAlignment(expenseCellTextAlignment());
    }
    if (m_subtitle) {
        m_subtitle->setText(tr_fixed_expenses_subtitle_a65f2a());
        m_subtitle->setAlignment(expenseCellTextAlignment());
    }
    if (m_groupLabel) m_groupLabel->setText(tr_group_by_2bda9d());
    if (m_graphBtn)
        m_graphBtn->setText(tr_show_graphs_26cf20());
    if (m_titleRow)
        m_titleRow->setDirection(isArabic() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);

    if (m_groupCombo) {
        const QSignalBlocker blocker(m_groupCombo);
        const int selected = m_groupCombo->currentData().isValid() ? m_groupCombo->currentData().toInt() : int(AccountTypeFilter::All);
        m_groupCombo->clear();
        m_groupCombo->addItem(tr_all_b4d286(), int(AccountTypeFilter::All));
        m_groupCombo->addItem(tr_account_receivable_59bf34(), int(AccountTypeFilter::Receivable));
        m_groupCombo->addItem(tr_account_payable_003206(), int(AccountTypeFilter::Payable));
        const int idx = m_groupCombo->findData(selected);
        if (idx >= 0)
            m_groupCombo->setCurrentIndex(idx);
        applyExpenseComboDirection(m_groupCombo);
    }

    for (auto& card : m_monthCards)
        updateMonthCardText(card);
    updateRowTexts();

    m_loadingRows = wasLoading;
    renderCurrentMonth();
}

AppData Accountswidget::collectData() const
{
    syncRowsToCurrentMonth();
    AppData d;
    d.monthlyAccounts = m_monthlyAccounts;

    // Keep an aggregated flat list for existing chart/PDF/import code paths that still read AppData::accounts.
    QStringList keys;
    QList<AccountItem> aggregated;
    for (int month = 0; month < 12; ++month) {
        const QList<AccountItem> list = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[month]);
        for (const auto& source : list) {
            AccountItem item = source;
            item.name = expenseAccountDisplayName(item);
            item.currency = (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"));
            const QString key = normalizedAccountKey(source);
            int pos = keys.indexOf(key);
            if (pos < 0) {
                keys << key;
                aggregated.append(item);
                pos = aggregated.size() - 1;
                aggregated[pos].amount = 0.0;
            }
            aggregated[pos].amount += source.amount;
            aggregated[pos].type = source.type;
        }
    }
    d.accounts = aggregated;
    d.calculate();
    return d;
}

void Accountswidget::setData(const AppData& data)
{
    initializeMonthData();
    if (hasAnyMonthlyExpenseAccounts(data)) {
        for (int month = 0; month < 12; ++month)
            m_monthlyAccounts[month] = normalizedFixedExpenseAccountsForMonth(data.monthlyAccounts[month]);
    } else if (!data.accounts.isEmpty()) {
        // Backward compatibility: old flat Expenses data is loaded into the first month.
        m_monthlyAccounts[0] = normalizedFixedExpenseAccountsForMonth(data.accounts);
    }

    renderCurrentMonth();
}

void Accountswidget::clearData()
{
    initializeMonthData();
    renderCurrentMonth();
    emit dataChanged();
}

bool Accountswidget::showGraphSelectionForRequest(const ChartRequest& request)
{
    AppData data = collectData();
    data.calculate();

    ChartKind kind = request.kind;
    if (kind == ChartKind::ComparePie)
        kind = ChartKind::Pie;
    else if (kind == ChartKind::CompareLine)
        kind = ChartKind::MetricLine;
    else if (kind == ChartKind::CompareBar || kind == ChartKind::MetricBar)
        kind = ChartKind::RankedBar;
    if (kind != ChartKind::Pie &&
        kind != ChartKind::RankedBar &&
        kind != ChartKind::MetricBar &&
        kind != ChartKind::MetricLine &&
        kind != ChartKind::HorizontalBar &&
        kind != ChartKind::Candle) {
        kind = ChartKind::RankedBar;
    }

    const bool hasExistingSelection = !request.months.isEmpty()
        || request.accountFilter != AccountTypeFilter::All
        || request.topAccountCount > 0
        || request.includeSummaryPoint
        || !request.title.trimmed().isEmpty();
    const ChartRequest* existing = hasExistingSelection ? &request : nullptr;
    ExpenseGraphSelectionDialog dlg(kind, data, this, existing);
    if (dlg.exec() != QDialog::Accepted)
        return false;

    ChartRequest req;
    req.origin = ChartOrigin::Accounts;
    req.kind = dlg.kind();
    req.metricA = M_EXPENSES;
    req.metricB = M_EXPENSES;
    req.accountFilter = dlg.accountFilter();
    req.topAccountCount = dlg.topAccountCount();
    req.months = dlg.selectedMonths();
    req.includeSummaryPoint = dlg.includeSummaryPoint();
    emit graphRequested(req);
    return true;
}

void Accountswidget::onShowGraphs()
{
    ChartRequest req;
    req.origin = ChartOrigin::Accounts;
    req.kind = ChartKind::RankedBar;
    req.metricA = M_EXPENSES;
    req.metricB = M_EXPENSES;
    req.accountFilter = AccountTypeFilter::All;
    req.topAccountCount = 10;
    showGraphSelectionForRequest(req);
}
