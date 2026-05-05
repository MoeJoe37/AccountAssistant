#include "SummaryWidget.h"
#include "translations.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QFrame>
#include <QAbstractItemView>
#include <QColor>
#include <QStringList>
#include <algorithm>

namespace {
static const char* kSummaryDark = R"(
QWidget#summaryRoot { background:#0d1020; }
QWidget#summaryHeader { background:#111526; border-bottom:1px solid #1a1f38; }
QLabel#summaryTitle { color:#c8d0ed; font-weight:900; font-size:18px; background:transparent; }
QLabel#summarySubtitle { color:#8a94bd; background:transparent; }
QFrame#summaryCard { background:#1a1f38; border:1px solid #252b52; border-radius:12px; }
QLabel#summaryCardTitle { color:#8a94bd; font-weight:800; background:transparent; }
QLabel#summaryCardValue { color:#e6ebff; font-weight:900; font-size:18px; background:transparent; }
QLabel#summarySectionTitle { color:#c8d0ed; font-weight:900; font-size:16px; background:transparent; }
QTableWidget { background:#111526; color:#c8d0ed; gridline-color:#252b52; border:1px solid #252b52; border-radius:10px; }
QHeaderView::section { background:#1a1f38; color:#4f86f7; border:none; padding:8px 10px; font-weight:800; }
QTableWidget::item { padding:7px; }
QTableWidget::item:selected { background:#1e2445; }
)";

static const char* kSummaryLight = R"(
QWidget#summaryRoot { background:#f4f6fb; }
QWidget#summaryHeader { background:#ffffff; border-bottom:1px solid #dde2f0; }
QLabel#summaryTitle { color:#1e2340; font-weight:900; font-size:18px; background:transparent; }
QLabel#summarySubtitle { color:#6b7280; background:transparent; }
QFrame#summaryCard { background:#ffffff; border:1px solid #dde2f0; border-radius:12px; }
QLabel#summaryCardTitle { color:#6b7280; font-weight:800; background:transparent; }
QLabel#summaryCardValue { color:#1e2340; font-weight:900; font-size:18px; background:transparent; }
QLabel#summarySectionTitle { color:#1e2340; font-weight:900; font-size:16px; background:transparent; }
QTableWidget { background:#ffffff; color:#1e2340; gridline-color:#dde2f0; border:1px solid #dde2f0; border-radius:10px; }
QHeaderView::section { background:#eef0fa; color:#1d4ed8; border:none; padding:8px 10px; font-weight:800; }
QTableWidget::item { padding:7px; }
QTableWidget::item:selected { background:#eef0fa; color:#1e2340; }
)";

static QTableWidgetItem* makeItem(const QString& text, const QColor& color = QColor())
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    if (color.isValid())
        item->setForeground(color);
    return item;
}

struct AccountSummaryRow {
    QString key;
    QString name;
    AccountType type = AccountType::Payable;
    double signedAmount = 0.0;
};
}

SummaryWidget::SummaryWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    retranslate();
    applyTheme();
    setData(AppData{});
}

void SummaryWidget::buildUi()
{
    setObjectName("summaryRoot");
    setLayoutDirection(appLayoutDirection());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    header->setObjectName("summaryHeader");
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(20, 16, 20, 14);
    headerLayout->setSpacing(8);
    m_title = new QLabel(header);
    m_title->setObjectName("summaryTitle");
    m_subtitle = new QLabel(header);
    m_subtitle->setObjectName("summarySubtitle");
    m_subtitle->setWordWrap(true);
    headerLayout->addWidget(m_title);
    headerLayout->addWidget(m_subtitle);
    root->addWidget(header);

    auto* body = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(20, 20, 20, 20);
    bodyLayout->setSpacing(18);

    auto* cards = new QWidget(body);
    auto* grid = new QGridLayout(cards);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(14);
    grid->setOriginCorner(isArabic() ? Qt::TopRightCorner : Qt::TopLeftCorner);

    auto makeCard = [&](QLabel*& title, QLabel*& value, int row, int col) {
        auto* card = new QFrame(cards);
        card->setObjectName("summaryCard");
        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(16, 14, 16, 14);
        layout->setSpacing(6);
        title = new QLabel(card);
        title->setObjectName("summaryCardTitle");
        title->setAlignment(appTextAlign() | Qt::AlignVCenter);
        value = new QLabel(QStringLiteral("—"), card);
        value->setObjectName("summaryCardValue");
        value->setAlignment(appTextAlign() | Qt::AlignVCenter);
        layout->addWidget(title);
        layout->addWidget(value);
        grid->addWidget(card, row, col);
    };
    makeCard(m_tradingTitle, m_tradingValue, 0, 0);
    makeCard(m_otherTitle, m_otherValue, 0, 1);
    makeCard(m_expensesTitle, m_expensesValue, 1, 0);
    makeCard(m_operatingTitle, m_operatingValue, 1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    bodyLayout->addWidget(cards);

    m_tableTitle = new QLabel(body);
    m_tableTitle->setObjectName("summarySectionTitle");
    bodyLayout->addWidget(m_tableTitle);

    m_table = new QTableWidget(body);
    m_table->setColumnCount(3);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    bodyLayout->addWidget(m_table, 1);

    root->addWidget(body, 1);
}

void SummaryWidget::setData(const AppData& data)
{
    m_data = data;
    m_data.calculate();
    updateCards();
    rebuildTable();
}

void SummaryWidget::clearData()
{
    setData(AppData{});
}

void SummaryWidget::updateCards()
{
    if (m_tradingValue) m_tradingValue->setText(formatMoneyText(m_data.totalProfit));
    if (m_otherValue) m_otherValue->setText(formatMoneyText(m_data.totalOtherRevenues));
    if (m_expensesValue) m_expensesValue->setText(formatMoneyText(m_data.totalSignedExpenses));
    if (m_operatingValue) m_operatingValue->setText(formatMoneyText(m_data.totalOperatingProfit));
    if (m_tradingValue) m_tradingValue->setStyleSheet(QStringLiteral("color:%1;background:transparent;font-weight:900;font-size:18px;").arg(metricColor(M_PROFIT_MARGIN).name()));
    if (m_otherValue) m_otherValue->setStyleSheet(QStringLiteral("color:#1f77b4;background:transparent;font-weight:900;font-size:18px;"));
    if (m_expensesValue) m_expensesValue->setStyleSheet(QStringLiteral("color:%1;background:transparent;font-weight:900;font-size:18px;").arg(m_data.totalSignedExpenses >= 0.0 ? QStringLiteral("#2ca02c") : QStringLiteral("#d62728")));
    if (m_operatingValue) m_operatingValue->setStyleSheet(QStringLiteral("color:%1;background:transparent;font-weight:900;font-size:18px;").arg(m_data.totalOperatingProfit >= 0.0 ? QStringLiteral("#2ca02c") : QStringLiteral("#d62728")));
}

void SummaryWidget::rebuildTable()
{
    if (!m_table)
        return;

    QList<AccountSummaryRow> rows;
    QStringList keys;
    for (int month = 0; month < 12; ++month) {
        const QList<AccountItem> list = normalizedFixedExpenseAccountsForMonth(m_data.monthlyAccounts[month]);
        for (const auto& item : list) {
            const QString label = expenseAccountDisplayName(item);
            if (label.trimmed().isEmpty())
                continue;
            const QString key = normalizedAccountKey(item);
            int pos = keys.indexOf(key);
            if (pos < 0) {
                keys << key;
                AccountSummaryRow row;
                row.key = key;
                row.name = label;
                row.type = item.type;
                rows << row;
                pos = rows.size() - 1;
            }
            rows[pos].type = item.type;
            rows[pos].signedAmount += (item.type == AccountType::Receivable) ? item.amount : -item.amount;
        }
    }

    std::stable_sort(rows.begin(), rows.end(), [](const AccountSummaryRow& a, const AccountSummaryRow& b) {
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });

    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r) {
        const auto& row = rows[r];
        const QColor amountColor = row.signedAmount >= 0.0 ? QColor("#2ca02c") : QColor("#d62728");
        m_table->setItem(r, 0, makeItem(row.name));
        m_table->setItem(r, 1, makeItem(row.type == AccountType::Receivable ? tr_account_receivable_59bf34() : tr_account_payable_003206()));
        m_table->setItem(r, 2, makeItem(formatMoneyText(row.signedAmount), amountColor));
        m_table->item(r, 0)->setTextAlignment(appTextAlign() | Qt::AlignVCenter);
        m_table->item(r, 1)->setTextAlignment(appTextAlign() | Qt::AlignVCenter);
        m_table->item(r, 2)->setTextAlignment((isArabic() ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignVCenter);
    }
}

void SummaryWidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kSummaryLight : kSummaryDark);
}

void SummaryWidget::retranslate()
{
    setLayoutDirection(appLayoutDirection());
    if (m_title) m_title->setText(tr_summary_title_891b2d());
    if (m_subtitle) m_subtitle->setText(tr_summary_subtitle_9bd0cf());
    if (m_tradingTitle) m_tradingTitle->setText(tr_trading_result_b21619());
    if (m_otherTitle) m_otherTitle->setText(tr_other_revenues_total_d457cf());
    if (m_expensesTitle) m_expensesTitle->setText(tr_expenses_total_signed_0f255b());
    if (m_operatingTitle) m_operatingTitle->setText(tr_operating_profit_c87e52());
    if (m_tableTitle) m_tableTitle->setText(tr_expenses_13597e());
    for (auto* label : {m_tradingTitle, m_tradingValue, m_otherTitle, m_otherValue, m_expensesTitle, m_expensesValue, m_operatingTitle, m_operatingValue}) {
        if (label)
            label->setAlignment(appTextAlign() | Qt::AlignVCenter);
    }
    if (m_table) {
        m_table->setHorizontalHeaderLabels(QStringList{tr_fixed_expense_account_header_a13bcd(), tr_expense_account_type_field_a870c9(), tr_account_result_e5033d()});
        m_table->setLayoutDirection(appLayoutDirection());
    }
    updateCards();
    rebuildTable();
}
