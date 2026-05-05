#include "Accountswidget.h"
#include "translations.h"
#include "themebox.h"

#include <QAbstractSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QInputDialog>
#include <QDialog>
#include <QLineEdit>
#include <QMenu>
#include <QSignalBlocker>
#include <QTimer>
#include <QWheelEvent>
#include <QAbstractItemView>
#include <QLocale>

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

    m_title = new QLabel(header);
    m_title->setObjectName("expensesTitle");
    m_subtitle = new QLabel(header);
    m_subtitle->setObjectName("expensesSubtitle");
    m_subtitle->setWordWrap(true);

    headerLayout->addWidget(m_title);
    headerLayout->addWidget(m_subtitle);

    auto* selectorRow = new QHBoxLayout;
    selectorRow->setDirection(isArabic() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);
    selectorRow->setSpacing(10);

    m_monthLabel = new QLabel(header);
    m_monthLabel->setObjectName("expensesLabel");
    m_monthCombo = new NoWheelComboBox(header);
    m_monthCombo->setMinimumWidth(180);
    styleComboPopup(m_monthCombo);

    const QStringList months = monthNames();
    for (int i = 0; i < 12; ++i)
        m_monthCombo->addItem(months.value(i), i);

    m_groupLabel = new QLabel(header);
    m_groupLabel->setObjectName("expensesLabel");
    m_groupCombo = new NoWheelComboBox(header);
    m_groupCombo->setMinimumWidth(200);
    styleComboPopup(m_groupCombo);
    m_groupCombo->addItem(tr_all_b4d286(), int(AccountTypeFilter::All));
    m_groupCombo->addItem(tr_account_receivable_59bf34(), int(AccountTypeFilter::Receivable));
    m_groupCombo->addItem(tr_account_payable_003206(), int(AccountTypeFilter::Payable));

    selectorRow->addWidget(m_monthLabel);
    selectorRow->addWidget(m_monthCombo);
    selectorRow->addSpacing(20);
    selectorRow->addWidget(m_groupLabel);
    selectorRow->addWidget(m_groupCombo);
    selectorRow->addStretch();
    headerLayout->addLayout(selectorRow);
    root->addWidget(header);

    auto* tableHeader = new QWidget(this);
    tableHeader->setObjectName("expensesHeader");
    m_tableHeaderLayout = new QGridLayout(tableHeader);
    m_tableHeaderLayout->setContentsMargins(34, 10, 34, 10);
    m_tableHeaderLayout->setHorizontalSpacing(12);
    m_tableHeaderLayout->setVerticalSpacing(0);
    m_tableHeaderLayout->setOriginCorner(isArabic() ? Qt::TopRightCorner : Qt::TopLeftCorner);

    m_accountHeader = new QLabel(tableHeader);
    m_accountHeader->setObjectName("expensesTableHeader");
    m_accountHeader->setAlignment(appTextAlign() | Qt::AlignVCenter);
    m_amountHeader = new QLabel(tableHeader);
    m_amountHeader->setObjectName("expensesTableHeader");
    m_amountHeader->setAlignment(appTextAlign() | Qt::AlignVCenter);
    m_typeHeader = new QLabel(tableHeader);
    m_typeHeader->setObjectName("expensesTableHeader");
    m_typeHeader->setAlignment(appTextAlign() | Qt::AlignVCenter);

    m_tableHeaderLayout->addWidget(m_accountHeader, 0, 0);
    m_tableHeaderLayout->addWidget(m_amountHeader, 0, 1);
    m_tableHeaderLayout->addWidget(m_typeHeader, 0, 2);
    m_tableHeaderLayout->setColumnStretch(0, 2);
    m_tableHeaderLayout->setColumnStretch(1, 1);
    m_tableHeaderLayout->setColumnStretch(2, 1);
    root->addWidget(tableHeader);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("expensesScroll");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget;
    m_container->setObjectName("expensesContainer");
    m_container->setAttribute(Qt::WA_StyledBackground, true);
    m_rowsLayout = new QVBoxLayout(m_container);
    m_rowsLayout->setContentsMargins(20, 16, 20, 20);
    m_rowsLayout->setSpacing(9);
    m_rowsLayout->addStretch();

    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll, 1);

    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_loadingRows)
            return;
        syncRowsToCurrentMonth();
        m_currentMonth = m_monthCombo ? m_monthCombo->currentData().toInt() : 0;
        renderCurrentMonth();
    });
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

void Accountswidget::deleteAccountAtIndex(int accountIndex)
{
    syncRowsToCurrentMonth();
    if (m_currentMonth < 0 || m_currentMonth >= 12)
        return;
    const QList<AccountItem> current = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[m_currentMonth]);
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
}

void Accountswidget::syncRowsToCurrentMonth() const
{
    if (m_currentMonth < 0 || m_currentMonth >= 12)
        return;

    QList<AccountItem> month = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[m_currentMonth]);
    for (const auto& row : m_rows) {
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
    m_monthlyAccounts[m_currentMonth] = month;
}

void Accountswidget::clearRows()
{
    while (!m_rows.isEmpty()) {
        RowWidgets row = m_rows.takeLast();
        if (row.row) {
            if (m_rowsLayout)
                m_rowsLayout->removeWidget(row.row);
            delete row.row;
        }
    }
}

void Accountswidget::renderCurrentMonth()
{
    clearRows();
    m_loadingRows = true;

    const int month = qBound(0, m_currentMonth, 11);
    QList<AccountItem> items = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[month]);
    m_monthlyAccounts[month] = items;
    const AccountTypeFilter filter = currentGroupFilter();

    for (int i = 0; i < items.size(); ++i) {
        const AccountItem& item = items[i];
        if (!accountMatchesFilter(item.type, filter))
            continue;

        auto* rowW = new QWidget(m_container);
        rowW->setObjectName("expenseRow");
        rowW->setLayoutDirection(appLayoutDirection());
        rowW->setContextMenuPolicy(Qt::CustomContextMenu);

        auto* rowLayout = new QGridLayout(rowW);
        rowLayout->setContentsMargins(14, 10, 14, 10);
        rowLayout->setHorizontalSpacing(12);
        rowLayout->setVerticalSpacing(0);
        if (isArabic())
            rowLayout->setOriginCorner(Qt::TopRightCorner);

        auto* accountLabel = new QLabel(expenseAccountDisplayName(item), rowW);
        accountLabel->setObjectName("expenseAccountName");
        accountLabel->setAlignment(appTextAlign() | Qt::AlignVCenter);
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
        amountSpin->setAlignment(appTextAlign() | Qt::AlignVCenter);
        amountSpin->setPrefix(currencyPrefix());
        amountSpin->setSuffix(currencySuffix());

        auto* typeCombo = new NoWheelComboBox(rowW);
        typeCombo->addItem(accountTypeLabel(AccountType::Receivable), int(AccountType::Receivable));
        typeCombo->addItem(accountTypeLabel(AccountType::Payable), int(AccountType::Payable));
        setComboToAccountType(typeCombo, item.type);
        typeCombo->setMinimumWidth(180);
        styleComboPopup(typeCombo);

        rowLayout->addWidget(accountLabel, 0, 0);
        rowLayout->addWidget(amountSpin, 0, 1);
        rowLayout->addWidget(typeCombo, 0, 2);
        rowLayout->setColumnStretch(0, 2);
        rowLayout->setColumnStretch(1, 1);
        rowLayout->setColumnStretch(2, 1);

        RowWidgets widgets;
        widgets.row = rowW;
        widgets.accountLabel = accountLabel;
        widgets.amountCaption = nullptr;
        widgets.typeCaption = nullptr;
        widgets.amount = amountSpin;
        widgets.type = typeCombo;
        widgets.accountIndex = i;
        m_rows.append(widgets);

        connect(rowW, &QWidget::customContextMenuRequested, this, [this, i, rowW](const QPoint& pos) {
            QMenu menu;
            menu.setStyleSheet(g_lightMode
                ? QStringLiteral("QMenu{background:#ffffff;color:#1e2340;border:1px solid #d9e0ef;padding:4px;}QMenu::item{padding:7px 22px;}QMenu::item:selected{background:#eef0fa;color:#1e2340;}")
                : QStringLiteral("QMenu{background:#1a1f38;color:#e7ecff;border:1px solid #343c63;padding:4px;}QMenu::item{padding:7px 22px;}QMenu::item:selected{background:#4f86f7;color:#ffffff;}"));
            QAction* del = menu.addAction(tr_delete_account_6dd013());
            QAction* chosen = menu.exec(rowW->mapToGlobal(pos));
            if (chosen == del)
                QTimer::singleShot(0, this, [this, i]() { deleteAccountAtIndex(i); });
        });

        connect(amountSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, i](double value) {
            if (m_loadingRows || m_currentMonth < 0 || m_currentMonth >= 12)
                return;
            QList<AccountItem> month = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[m_currentMonth]);
            if (i >= 0 && i < month.size()) {
                month[i].amount = value;
                m_monthlyAccounts[m_currentMonth] = month;
            }
        });
        connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, i](int) {
            if (m_loadingRows || m_currentMonth < 0 || m_currentMonth >= 12)
                return;
            QList<AccountItem> month = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[m_currentMonth]);
            if (i >= 0 && i < month.size()) {
                const RowWidgets* found = nullptr;
                for (const auto& r : m_rows) {
                    if (r.accountIndex == i) { found = &r; break; }
                }
                if (found && found->type)
                    month[i].type = accountTypeFromCombo(found->type);
                m_monthlyAccounts[m_currentMonth] = month;
                if (!accountMatchesFilter(month[i].type, currentGroupFilter()))
                    renderCurrentMonth();
            }
        });

        m_rowsLayout->insertWidget(qMax(0, m_rowsLayout->count() - 1), rowW);
    }

    m_loadingRows = false;
}

void Accountswidget::updateRowTexts()
{
    const int month = qBound(0, m_currentMonth, 11);
    const QList<AccountItem> items = normalizedFixedExpenseAccountsForMonth(m_monthlyAccounts[month]);
    for (auto& row : m_rows) {
        if (row.accountLabel && row.accountIndex >= 0 && row.accountIndex < items.size())
            row.accountLabel->setText(expenseAccountDisplayName(items.value(row.accountIndex)));
        if (row.amountCaption)
            row.amountCaption->setText(tr_expense_amount_field_93a771());
        if (row.typeCaption)
            row.typeCaption->setText(tr_expense_account_type_field_a870c9());
        if (row.amount) {
            row.amount->setPrefix(currencyPrefix());
            row.amount->setSuffix(currencySuffix());
            row.amount->setDecimals(currencyDecimals());
            row.amount->setSingleStep(g_currency == AppCurrency::IQD ? 1000.0 : 100.0);
        }
        if (row.type) {
            const AccountType selected = accountTypeFromCombo(row.type);
            const QSignalBlocker blocker(row.type);
            row.type->clear();
            row.type->addItem(accountTypeLabel(AccountType::Receivable), int(AccountType::Receivable));
            row.type->addItem(accountTypeLabel(AccountType::Payable), int(AccountType::Payable));
            setComboToAccountType(row.type, selected);
        }
    }
}

void Accountswidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kExpensesLight : kExpensesDark);
    if (m_container)
        m_container->setStyleSheet(g_lightMode ? QStringLiteral("background:#f4f6fb;") : QStringLiteral("background:#0d1020;"));
}

void Accountswidget::retranslate()
{
    const bool wasLoading = m_loadingRows;
    m_loadingRows = true;
    syncRowsToCurrentMonth();
    setLayoutDirection(appLayoutDirection());

    if (m_title) m_title->setText(tr_expenses_13597e());
    if (m_subtitle) m_subtitle->setText(tr_fixed_expenses_subtitle_a65f2a());
    if (m_monthLabel) m_monthLabel->setText(tr_expense_months_dropdown_label_62ac11());
    if (m_groupLabel) m_groupLabel->setText(tr_group_by_2bda9d());
    if (m_tableHeaderLayout)
        m_tableHeaderLayout->setOriginCorner(isArabic() ? Qt::TopRightCorner : Qt::TopLeftCorner);
    if (m_accountHeader) {
        m_accountHeader->setText(tr_fixed_expense_account_header_a13bcd());
        m_accountHeader->setAlignment(appTextAlign() | Qt::AlignVCenter);
    }
    if (m_amountHeader) {
        m_amountHeader->setText(tr_expense_amount_field_93a771());
        m_amountHeader->setAlignment(appTextAlign() | Qt::AlignVCenter);
    }
    if (m_typeHeader) {
        m_typeHeader->setText(tr_expense_account_type_field_a870c9());
        m_typeHeader->setAlignment(appTextAlign() | Qt::AlignVCenter);
    }

    if (m_monthCombo) {
        const QSignalBlocker blocker(m_monthCombo);
        const int selectedMonth = m_monthCombo->currentData().isValid() ? m_monthCombo->currentData().toInt() : m_currentMonth;
        m_monthCombo->clear();
        const QStringList months = monthNames();
        for (int i = 0; i < 12; ++i)
            m_monthCombo->addItem(months.value(i), i);
        const int idx = m_monthCombo->findData(qBound(0, selectedMonth, 11));
        if (idx >= 0)
            m_monthCombo->setCurrentIndex(idx);
    }

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
    }

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

    m_currentMonth = qBound(0, m_currentMonth, 11);
    if (m_monthCombo) {
        const QSignalBlocker blocker(m_monthCombo);
        const int idx = m_monthCombo->findData(m_currentMonth);
        if (idx >= 0) m_monthCombo->setCurrentIndex(idx);
    }
    renderCurrentMonth();
}

void Accountswidget::clearData()
{
    initializeMonthData();
    renderCurrentMonth();
}

bool Accountswidget::showGraphSelectionForRequest(const ChartRequest&)
{
    return false;
}
