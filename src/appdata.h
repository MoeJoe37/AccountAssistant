#pragma once
#include <array>
#include <algorithm>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QVector>
#include <QColor>
#include <QtGlobal>
#include "translations.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Per-month data (exactly what the user enters in the table)
// ─────────────────────────────────────────────────────────────────────────────
struct MonthData {
    double sales             = 0.0;
    double salesReturn       = 0.0;
    double supplierPurchases = 0.0;
    double supplierPayments  = 0.0;
    QString supplierName;
    QString expenseAccount;
    double expenseAmount     = 0.0;
    double inventoryFirst    = 0.0;
    double inventoryLast     = 0.0;
    double cogsInput         = 0.0;
};

struct SupplierMonthData {
    QString supplierName;
    double  purchases = 0.0;
    double  payments  = 0.0;
};

struct OtherRevenueMonthData {
    double acquiredPrivileges = 0.0;
    double miscellaneousRevenues = 0.0;

    double total() const
    {
        return acquiredPrivileges + miscellaneousRevenues;
    }
};

struct SupplierEntry {
    QString name;
    double previousBalance = 0.0;
    double purchases = 0.0;
    double totalDebt = 0.0;
    double payments = 0.0;

    double paymentPctOfPurchases() const
    {
        return purchases > 0.0 ? (payments / purchases) * 100.0 : 0.0;
    }

    double paymentPctOfTotalDebt() const
    {
        return totalDebt > 0.0 ? (payments / totalDebt) * 100.0 : 0.0;
    }

    double supplierBalance() const
    {
        return totalDebt - payments;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Expense/account entry independent from months
// ─────────────────────────────────────────────────────────────────────────────
enum class AccountType {
    // Keep the legacy values stable so older locally saved data remains readable.
    Payable = 0,
    Receivable = 1,
    Capital = 2,
    CostsOfRevenue = 3,
    FixedAssets = 4,
    CurrentYearsEarnings = 5,
    Expenses = 6,
    PrepaidPayments = 7,
    Income = 8,
    BankAndCash = 9,
    CurrentAssets = 10,
    NonCurrentLiabilities = 11,
    CurrentLiabilities = 12
};

enum class AccountTypeFilter {
    All = -1,
    Payable = 0,
    Receivable = 1,
    Capital = 2,
    CostsOfRevenue = 3,
    FixedAssets = 4,
    CurrentYearsEarnings = 5,
    Expenses = 6,
    PrepaidPayments = 7,
    Income = 8,
    BankAndCash = 9,
    CurrentAssets = 10,
    NonCurrentLiabilities = 11,
    CurrentLiabilities = 12
};

struct AccountItem {
    QString code;
    QString name;
    QString currency;
    bool allowSettlement = false;
    double amount = 0.0; // amount for the selected month in the fixed expenses grid
    AccountType type = AccountType::Payable;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Expense account aggregated for charts / PDF
// ─────────────────────────────────────────────────────────────────────────────
struct ExpenseSummary {
    QString account;
    double  total = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Chart-type selection per metric (legacy UI compatibility)
// ─────────────────────────────────────────────────────────────────────────────
struct ChartSel {
    bool pie    = false;
    bool candle = false;
    bool bar    = false;
    bool line   = false;
};

enum MetricId {
    M_SALES = 0,
    M_SALES_RETURN,
    M_PURCHASES,
    M_EXPENSES,
    M_INVENTORY,
    M_NET_SALES,
    M_COGS,
    M_PROFIT_MARGIN,
    M_SUPPLIER_PAYMENTS,
    M_SUPPLIER_PREVIOUS_BALANCE,
    M_SUPPLIER_TOTAL_DEBT,
    M_SUPPLIER_PAYMENT_PCT_PURCHASES,
    M_SUPPLIER_PAYMENT_PCT_DEBT,
    M_SUPPLIER_BALANCE,
    M_EXPENSE_AMOUNT,
    M_INVENTORY_OPENING,
    M_INVENTORY_CLOSING,
    M_COGS_VS_PROFIT,
    M_SUPPLIER_NAME,
    M_COUNT
};

enum class ChartKind {
    Pie,
    Candle,
    CompareBar,
    CompareLine,
    ComparePie,
    RankedBar,
    MetricBar,
    MetricLine,
    HorizontalBar
};

enum class InventoryMode {
    Periodic = 0,
    Ongoing = 1
};

enum class CompareGroup {
    General = 0,
    Accounts = 1,
    Suppliers = 2
};

enum class ChartOrigin {
    Custom = 0,
    Accounts = 1,
    Suppliers = 2
};

struct ChartRequest {
    ChartKind kind = ChartKind::Candle;
    MetricId metricA = M_SALES;
    MetricId metricB = M_SALES_RETURN;
    QList<MetricId> compareMetrics;  // Ordered graph metrics for comparison charts.
    bool axisMetricsAuto = true;
    MetricId xAxisMetric = M_COUNT;
    MetricId yAxisMetric = M_COUNT;
    MetricId comparePieBaseMetric = M_COUNT;  // Optional base metric for ComparePie (100%).
    QString title;
    QString seriesA;
    QString seriesB;
    QList<int> months;   // Empty means all months
    AccountTypeFilter accountFilter = AccountTypeFilter::All;
    int topAccountCount = 0; // 0 means show all accounts; used by Expenses top-account charts.
    bool includeSummaryPoint = false;
    ChartOrigin origin = ChartOrigin::Custom;
};

enum class ResultFlowItemKind {
    MonthCard,
    ChartCard,
    PageSeparator
};

struct ResultFlowItem {
    ResultFlowItemKind kind = ResultFlowItemKind::MonthCard;
    int index = -1;   // Month index or chart request index.
    int id = -1;      // Unique separator id when kind == PageSeparator.
};

inline bool operator==(const ResultFlowItem& a, const ResultFlowItem& b)
{
    return a.kind == b.kind && a.index == b.index && a.id == b.id;
}

inline bool operator!=(const ResultFlowItem& a, const ResultFlowItem& b)
{
    return !(a == b);
}


inline int fixedExpenseAccountIndexFromCode(const QString& code)
{
    const QString c = code.trimmed().toUpper();
    if (c.startsWith(QStringLiteral("FX"))) {
        bool ok = false;
        const int n = c.mid(2).toInt(&ok);
        if (ok && n >= 1 && n <= fixedExpenseAccountCount())
            return n - 1;
    }
    return -1;
}

inline QString normalizedAccountKey(const AccountItem& item)
{
    const QString code = item.code.trimmed().toUpper();
    if (!code.isEmpty())
        return QStringLiteral("CODE:") + code;
    return QStringLiteral("NAME:") + item.name.trimmed().toCaseFolded();
}

inline int fixedExpenseAccountIndexFromItem(const AccountItem& item)
{
    int idx = fixedExpenseAccountIndexFromCode(item.code);
    if (idx >= 0)
        return idx;

    const QString key = item.name.trimmed().toCaseFolded();
    if (key.isEmpty())
        return -1;

    const QStringList names = fixedExpenseAccountNames();
    for (int i = 0; i < names.size(); ++i) {
        if (names.value(i).trimmed().toCaseFolded() == key)
            return i;
    }

    // Backward-compatible matching for saved/imported fixed accounts that were
    // stored in the other UI language. Keep this intentionally literal so it
    // does not temporarily mutate the global language while widgets repaint.
    static const QStringList enNames = {
        QStringLiteral("Salaries and wages"),
        QStringLiteral("Monthly incentives"),
        QStringLiteral("Bonuses and allowances"),
        QStringLiteral("Fuel and oil"),
        QStringLiteral("Supplies and consumables"),
        QStringLiteral("Stationery"),
        QStringLiteral("Building maintenance"),
        QStringLiteral("Furniture maintenance"),
        QStringLiteral("Vehicle maintenance and transportation"),
        QStringLiteral("Advertising and publicity"),
        QStringLiteral("Transport, dispatch, and communications"),
        QStringLiteral("Transportation vehicle rental"),
        QStringLiteral("Building rent"),
        QStringLiteral("Other service expenses"),
        QStringLiteral("Bank expenses"),
        QStringLiteral("Gifts and donations"),
        QStringLiteral("Taxes and miscellaneous fees"),
        QStringLiteral("Distribution expenses"),
        QStringLiteral("Promotional allowance"),
        QStringLiteral("Cash allowance"),
        QStringLiteral("Price difference allowance"),
        QStringLiteral("Damaged goods allowance"),
        QStringLiteral("Gift allowance"),
        QStringLiteral("Damaged inventory")
    };
    static const QStringList arNames = {
        QString::fromUtf8("رواتب واجور"),
        QString::fromUtf8("الحوافز الشهرية"),
        QString::fromUtf8("مكافات واكراميات"),
        QString::fromUtf8("وقود وزيوت"),
        QString::fromUtf8("لوازم ومهمات"),
        QString::fromUtf8("قرطاسية"),
        QString::fromUtf8("صيانة مباني"),
        QString::fromUtf8("صيانة الاثاث"),
        QString::fromUtf8("صيانة وسائط نقل وانتقال"),
        QString::fromUtf8("دعاية واعلان"),
        QString::fromUtf8("نقل وايفاد واتصالات"),
        QString::fromUtf8("استئجار وسائط نقل وانتقال"),
        QString::fromUtf8("استئجار مباني"),
        QString::fromUtf8("مصروفات خدمية اخرى"),
        QString::fromUtf8("مصاريف بنك"),
        QString::fromUtf8("هدايا وتبرعات"),
        QString::fromUtf8("ضرائب ورسوم متنوعة"),
        QString::fromUtf8("مصاريف التوزيع"),
        QString::fromUtf8("سماح تشجيعي"),
        QString::fromUtf8("سماح نقدي"),
        QString::fromUtf8("سماح فرق السعر"),
        QString::fromUtf8("سماح تالف"),
        QString::fromUtf8("سماح هدايا"),
        QString::fromUtf8("تالف المخزون السلعي")
    };
    for (int i = 0; i < enNames.size(); ++i) {
        if (enNames.value(i).trimmed().toCaseFolded() == key ||
            arNames.value(i).trimmed().toCaseFolded() == key)
            return i;
    }
    return -1;
}

inline QString expenseAccountDisplayName(const AccountItem& item)
{
    const int idx = fixedExpenseAccountIndexFromItem(item);
    if (idx >= 0)
        return fixedExpenseAccountNames().value(idx);
    return item.name.trimmed();
}

inline QList<AccountItem> defaultFixedExpenseAccounts()
{
    QList<AccountItem> items;
    const QStringList names = fixedExpenseAccountNames();
    for (int i = 0; i < names.size(); ++i) {
        AccountItem item;
        item.code = fixedExpenseAccountCode(i);
        item.name = names.value(i);
        item.currency = (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"));
        item.type = AccountType::Payable;
        item.amount = 0.0;
        items.append(item);
    }
    return items;
}

struct AppData;
inline bool hasAnyMonthlyExpenseAccounts(const AppData& d);

inline QList<AccountItem> normalizedFixedExpenseAccountsForMonth(const QList<AccountItem>& source)
{
    QList<AccountItem> items = defaultFixedExpenseAccounts();
    QStringList customKeys;

    for (const auto& raw : source) {
        AccountItem clean = raw;
        clean.name = raw.name.trimmed();
        clean.code = raw.code.trimmed();
        clean.type = (raw.type == AccountType::Receivable) ? AccountType::Receivable : AccountType::Payable;
        clean.currency = raw.currency.trimmed().isEmpty()
            ? (g_currency == AppCurrency::USD ? QStringLiteral("USD") : QStringLiteral("IQD"))
            : raw.currency.trimmed().toUpper();

        const int idx = fixedExpenseAccountIndexFromItem(clean);
        if (idx >= 0 && idx < items.size()) {
            items[idx].amount = clean.amount;
            items[idx].type = clean.type;
            items[idx].currency = clean.currency;
            items[idx].allowSettlement = clean.allowSettlement;
            continue;
        }

        if (clean.name.isEmpty())
            continue;
        if (clean.code.isEmpty())
            clean.code = QStringLiteral("CX-%1").arg(clean.name.trimmed().toCaseFolded());

        const QString key = normalizedAccountKey(clean);
        int existing = -1;
        for (int i = 0; i < items.size(); ++i) {
            if (fixedExpenseAccountIndexFromItem(items[i]) < 0 && normalizedAccountKey(items[i]) == key) {
                existing = i;
                break;
            }
        }
        if (existing >= 0) {
            items[existing] = clean;
        } else if (!customKeys.contains(key)) {
            customKeys << key;
            items.append(clean);
        }
    }
    return items;
}


// ─────────────────────────────────────────────────────────────────────────────
//  Full application state
// ─────────────────────────────────────────────────────────────────────────────
struct AppData {
    std::array<MonthData, 12> months{};
    std::array<ChartSel, M_COUNT> sel{};
    QList<AccountItem> accounts; // legacy/aggregated account list
    std::array<QList<AccountItem>, 12> monthlyAccounts{};
    std::array<SupplierMonthData, 12> suppliers{};
    std::array<QList<SupplierEntry>, 12> supplierEntries{};
    std::array<OtherRevenueMonthData, 12> otherRevenues{};
    InventoryMode inventoryMode = InventoryMode::Periodic;

    // Chosen charts for the Results tab and PDF export
    QList<ChartRequest> chartRequests;
    QList<ChartRequest> hiddenChartRequests;
    QList<ResultFlowItem> resultFlowOrder;

    // Computed per-month
    std::array<double, 12> netSales{};
    std::array<double, 12> cogs{};
    std::array<double, 12> profitMargin{};
    std::array<double, 12> otherRevenueTotals{};
    std::array<double, 12> signedExpenses{};
    std::array<double, 12> operatingProfit{};

    // Grand totals (for summary bar)
    double totalNetSales = 0;
    double totalCOGS     = 0;
    double totalProfit   = 0;
    double totalOtherRevenues = 0;
    double totalSignedExpenses = 0;
    double totalOperatingProfit = 0;

    // Expense accounts aggregated & sorted by total (descending)
    QList<ExpenseSummary> expenseSummary;

    void calculate()
    {
        totalNetSales = totalCOGS = totalProfit = 0;
        totalOtherRevenues = totalSignedExpenses = totalOperatingProfit = 0;
        for (int i = 0; i < 12; ++i) {
            auto& m = months[i];
            netSales[i]     = m.sales - m.salesReturn;
            cogs[i]         = (inventoryMode == InventoryMode::Ongoing)
                                ? m.cogsInput
                                : (m.inventoryFirst + m.supplierPurchases - m.inventoryLast);
            profitMargin[i] = netSales[i] - cogs[i];

            totalNetSales += netSales[i];
            totalCOGS     += cogs[i];
            totalProfit   += profitMargin[i];
        }

        for (int i = 0; i < 12; ++i) {
            if (!supplierEntries[i].isEmpty()) {
                double totalPurchases = 0.0;
                double totalPayments = 0.0;
                QString firstName;
                for (auto& e : supplierEntries[i]) {
                    if (e.totalDebt <= 0.0)
                        e.totalDebt = e.previousBalance + e.purchases;
                    totalPurchases += e.purchases;
                    totalPayments += e.payments;
                    if (firstName.isEmpty() && !e.name.trimmed().isEmpty())
                        firstName = e.name.trimmed();
                }
                suppliers[i].supplierName = firstName;
                suppliers[i].purchases = totalPurchases;
                suppliers[i].payments = totalPayments;
            }
        }

        for (int i = 0; i < 12; ++i) {
            otherRevenueTotals[i] = otherRevenues[i].total();

            double signedExpense = 0.0;
            const QList<AccountItem> expenseList = normalizedFixedExpenseAccountsForMonth(monthlyAccounts[i]);
            for (const auto& item : expenseList)
                signedExpense += (item.type == AccountType::Receivable) ? item.amount : -item.amount;
            signedExpenses[i] = signedExpense;

            operatingProfit[i] = profitMargin[i] + otherRevenueTotals[i] + signedExpenses[i];
            totalOtherRevenues += otherRevenueTotals[i];
            totalSignedExpenses += signedExpenses[i];
            totalOperatingProfit += operatingProfit[i];
        }

        // Build expense summary from the monthly Expenses tab.
        // Custom user-added accounts are preserved and aggregated together with fixed accounts.
        expenseSummary.clear();
        if (hasAnyMonthlyExpenseAccounts(*this)) {
            QStringList keys;
            QStringList labels;
            QVector<double> totals;
            for (int month = 0; month < 12; ++month) {
                const QList<AccountItem> list = normalizedFixedExpenseAccountsForMonth(monthlyAccounts[month]);
                for (const auto& item : list) {
                    if (item.amount == 0.0 && fixedExpenseAccountIndexFromItem(item) >= 0)
                        continue;
                    const QString label = expenseAccountDisplayName(item);
                    if (label.isEmpty())
                        continue;
                    const QString key = normalizedAccountKey(item);
                    int pos = keys.indexOf(key);
                    if (pos < 0) {
                        keys << key;
                        labels << label;
                        totals << 0.0;
                        pos = totals.size() - 1;
                    }
                    totals[pos] += item.amount;
                }
            }
            for (int i = 0; i < totals.size(); ++i) {
                const bool customAccount = !keys.value(i).startsWith(QStringLiteral("CODE:FX"));
                if (totals[i] != 0.0 || customAccount)
                    expenseSummary.append({labels.value(i), totals.value(i)});
            }
        } else if (!accounts.isEmpty()) {
            for (const auto& a : accounts) {
                const QString name = expenseAccountDisplayName(a);
                if (!name.isEmpty() && a.amount != 0.0)
                    expenseSummary.append({name, a.amount});
            }
        } else {
            for (const auto& m : months) {
                const QString acc = m.expenseAccount.trimmed();
                if (!acc.isEmpty() && m.expenseAmount != 0.0)
                    expenseSummary.append({acc, m.expenseAmount});
            }
        }
    }
};

inline bool hasAnyMonthlyExpenseAccounts(const AppData& d)
{
    for (const auto& list : d.monthlyAccounts) {
        if (!list.isEmpty())
            return true;
    }
    return false;
}

inline double monthlyExpenseAccountTotal(const AppData& d, int monthIndex, AccountTypeFilter filter = AccountTypeFilter::All)
{
    if (monthIndex < 0 || monthIndex >= 12)
        return 0.0;
    double total = 0.0;
    const QList<AccountItem> list = normalizedFixedExpenseAccountsForMonth(d.monthlyAccounts[monthIndex]);
    for (const auto& item : list) {
        if (filter == AccountTypeFilter::All || static_cast<int>(item.type) == static_cast<int>(filter))
            total += item.amount;
    }
    return total;
}

inline double monthlyExpenseAccountSignedTotal(const AppData& d, int monthIndex, AccountTypeFilter filter = AccountTypeFilter::All)
{
    if (monthIndex < 0 || monthIndex >= 12)
        return 0.0;
    double total = 0.0;
    const QList<AccountItem> list = normalizedFixedExpenseAccountsForMonth(d.monthlyAccounts[monthIndex]);
    for (const auto& item : list) {
        if (filter == AccountTypeFilter::All || static_cast<int>(item.type) == static_cast<int>(filter))
            total += (item.type == AccountType::Receivable) ? item.amount : -item.amount;
    }
    return total;
}

inline double otherRevenueTotalForMonth(const AppData& d, int monthIndex)
{
    if (monthIndex < 0 || monthIndex >= 12)
        return 0.0;
    return d.otherRevenues[monthIndex].total();
}


inline bool appDataHasUserEntries(const AppData& d)
{
    for (const auto& m : d.months) {
        if (m.sales != 0.0 || m.salesReturn != 0.0 || m.supplierPurchases != 0.0 ||
            m.supplierPayments != 0.0 || !m.supplierName.trimmed().isEmpty() ||
            !m.expenseAccount.trimmed().isEmpty() || m.expenseAmount != 0.0 ||
            m.inventoryFirst != 0.0 || m.inventoryLast != 0.0 || m.cogsInput != 0.0)
            return true;
    }

    if (!d.chartRequests.isEmpty() || !d.hiddenChartRequests.isEmpty() || !d.resultFlowOrder.isEmpty())
        return true;

    for (const auto& a : d.accounts) {
        if (a.amount != 0.0 || fixedExpenseAccountIndexFromItem(a) < 0)
            return true;
    }

    for (const auto& list : d.monthlyAccounts) {
        for (const auto& a : list) {
            if (a.amount != 0.0 || (fixedExpenseAccountIndexFromItem(a) < 0 && !a.name.trimmed().isEmpty()))
                return true;
        }
    }

    for (const auto& r : d.otherRevenues) {
        if (r.acquiredPrivileges != 0.0 || r.miscellaneousRevenues != 0.0)
            return true;
    }

    for (const auto& s : d.suppliers) {
        if (!s.supplierName.trimmed().isEmpty() || s.purchases != 0.0 || s.payments != 0.0)
            return true;
    }

    for (const auto& monthEntries : d.supplierEntries) {
        if (!monthEntries.isEmpty()) {
            for (const auto& e : monthEntries) {
                if (!e.name.trimmed().isEmpty() || e.previousBalance != 0.0 || e.purchases != 0.0 ||
                    e.totalDebt != 0.0 || e.payments != 0.0)
                    return true;
            }
        }
    }

    return false;
}

inline QList<AccountType> accountTypesInUiOrder()
{
    return {
        AccountType::Capital,
        AccountType::CostsOfRevenue,
        AccountType::FixedAssets,
        AccountType::CurrentYearsEarnings,
        AccountType::Expenses,
        AccountType::Receivable,
        AccountType::PrepaidPayments,
        AccountType::Income,
        AccountType::Payable,
        AccountType::BankAndCash,
        AccountType::CurrentAssets,
        AccountType::NonCurrentLiabilities,
        AccountType::CurrentLiabilities
    };
}

inline QString accountTypeDisplayName(AccountType type)
{
    switch (type) {
    case AccountType::Capital:               return T("Capital", "رأس المال");
    case AccountType::CostsOfRevenue:        return T("Costs of Revenue", "تكاليف الإيرادات");
    case AccountType::FixedAssets:           return T("Fixed Assets", "الأصول الثابتة");
    case AccountType::CurrentYearsEarnings:  return T("Current Year's Earnings", "أرباح السنة الحالية");
    case AccountType::Expenses:              return T("Expenses", "المصروفات");
    case AccountType::Receivable:            return T("Receivable", "حسابات مدينة");
    case AccountType::PrepaidPayments:       return T("Prepaid Payments", "مدفوعات مقدمة");
    case AccountType::Income:                return T("Income", "الدخل");
    case AccountType::Payable:               return T("Payable", "حسابات دائنة");
    case AccountType::BankAndCash:           return T("Bank and Cash", "البنك والنقد");
    case AccountType::CurrentAssets:         return T("Current Assets", "الأصول المتداولة");
    case AccountType::NonCurrentLiabilities: return T("Non-Current Liabilities", "الالتزامات غير المتداولة");
    case AccountType::CurrentLiabilities:    return T("Current Liabilities", "الالتزامات المتداولة");
    }
    return T("Payable", "حسابات دائنة");
}

inline AccountType accountTypeFromText(const QString& text)
{
    QString k = text.trimmed().toCaseFolded();
    k.remove(QChar('\''));
    k.replace(QChar('_'), QChar(' '));
    k.replace(QChar('-'), QChar(' '));
    while (k.contains(QStringLiteral("  ")))
        k.replace(QStringLiteral("  "), QStringLiteral(" "));

    if (k.contains(QStringLiteral("capital")) || k.contains(QStringLiteral("رأس المال"))) return AccountType::Capital;
    if (k.contains(QStringLiteral("costs of revenue")) || k.contains(QStringLiteral("cost of revenue")) || k.contains(QStringLiteral("تكاليف الإيرادات"))) return AccountType::CostsOfRevenue;
    if (k.contains(QStringLiteral("fixed assets")) || k.contains(QStringLiteral("الأصول الثابتة")) || k.contains(QStringLiteral("أصول ثابتة"))) return AccountType::FixedAssets;
    if (k.contains(QStringLiteral("current years earnings")) || k.contains(QStringLiteral("current year earnings")) || k.contains(QStringLiteral("أرباح السنة"))) return AccountType::CurrentYearsEarnings;
    if (k == QStringLiteral("expenses") || k.contains(QStringLiteral("expense")) || k.contains(QStringLiteral("المصروف"))) return AccountType::Expenses;
    if (k.contains(tr_import_account_type_receivable_ar_keyword()) || k.contains(QStringLiteral("receivable")) || k.contains(QStringLiteral("حسابات مدينة"))) return AccountType::Receivable;
    if (k.contains(QStringLiteral("prepaid payments")) || k.contains(QStringLiteral("prepaid")) || k.contains(QStringLiteral("مدفوعات مقدمة"))) return AccountType::PrepaidPayments;
    if (k.contains(QStringLiteral("income")) || k.contains(QStringLiteral("الدخل"))) return AccountType::Income;
    if (k.contains(QStringLiteral("bank and cash")) || k.contains(QStringLiteral("cash")) || k.contains(QStringLiteral("bank")) || k.contains(QStringLiteral("البنك")) || k.contains(QStringLiteral("النقد"))) return AccountType::BankAndCash;
    if (k.contains(QStringLiteral("current assets")) || k.contains(QStringLiteral("الأصول المتداولة"))) return AccountType::CurrentAssets;
    if (k.contains(QStringLiteral("non current liabilities")) || k.contains(QStringLiteral("noncurrent liabilities")) || k.contains(QStringLiteral("الالتزامات غير المتداولة"))) return AccountType::NonCurrentLiabilities;
    if (k.contains(QStringLiteral("current liabilities")) || k.contains(QStringLiteral("الالتزامات المتداولة"))) return AccountType::CurrentLiabilities;
    if (k.contains(QStringLiteral("payable")) || k.contains(QStringLiteral("حسابات دائنة"))) return AccountType::Payable;
    return AccountType::Payable;
}

inline AccountTypeFilter accountTypeFilterFromType(AccountType type)
{
    return static_cast<AccountTypeFilter>(static_cast<int>(type));
}

inline bool accountMatchesFilter(AccountType type, AccountTypeFilter filter)
{
    return filter == AccountTypeFilter::All || static_cast<int>(type) == static_cast<int>(filter);
}

inline QString accountTypeFilterDisplayName(AccountTypeFilter type)
{
    if (type == AccountTypeFilter::All)
        return tr_all_b4d286();
    return accountTypeDisplayName(static_cast<AccountType>(static_cast<int>(type)));
}

inline QString metricDisplayName(MetricId id)
{
    switch (id) {
    case M_SALES:            return tr_sales_8fb4a6();
    case M_SALES_RETURN:     return tr_sales_return_08f992();
    case M_PURCHASES:        return tr_purchases_513aec();
    case M_EXPENSES:         return tr_expenses_13597e();
    case M_INVENTORY:        return tr_inventory_f1213f();
    case M_NET_SALES:        return tr_net_sales_90f56d();
    case M_COGS:             return tr_cost_of_goods_sold_55196f();
    case M_PROFIT_MARGIN:    return tr_profit_margin_56b595();
    case M_SUPPLIER_PAYMENTS:return tr_supplier_payments_bb713e();
    case M_SUPPLIER_PREVIOUS_BALANCE:return tr_auto_supplier_previous_balance_bd51822f();
    case M_SUPPLIER_TOTAL_DEBT:return tr_auto_supplier_total_debt_26cc7be2();
    case M_SUPPLIER_PAYMENT_PCT_PURCHASES:return tr_auto_supplier_payment_of_purchases_76125d4c();
    case M_SUPPLIER_PAYMENT_PCT_DEBT:return tr_auto_supplier_payment_of_debt_869d769d();
    case M_SUPPLIER_BALANCE:return tr_auto_supplier_balance_74852681();
    case M_EXPENSE_AMOUNT:   return tr_expense_amount_1ad3d5();
    case M_INVENTORY_OPENING:return tr_inventory_opening_ccde20();
    case M_INVENTORY_CLOSING:return tr_inventory_closing_d69943();
    case M_COGS_VS_PROFIT:   return tr_cogs_vs_profit_margin_fd48e9();
    case M_SUPPLIER_NAME:    return tr_auto_supplier_name_ac45e726();
    case M_COUNT:            break;
    }
    return tr_unknown_0240b2();
}

inline bool metricUsesMonthlySeries(MetricId id)
{
    return id != M_EXPENSES && id != M_COGS_VS_PROFIT;
}

inline QList<double> metricSeriesValues(const AppData& d, MetricId id, QStringList* labels = nullptr, const QList<int>* monthFilter = nullptr, AccountTypeFilter accountFilter = AccountTypeFilter::All, int topAccountCount = 0)
{
    if (labels)
        labels->clear();

    const auto includeMonth = [&](int idx) {
        return !monthFilter || monthFilter->isEmpty() || monthFilter->contains(idx);
    };

    QList<double> values;
    auto applyTopAccountLimit = [&]() {
        if (id != M_EXPENSES || topAccountCount <= 0 || values.size() <= topAccountCount)
            return;
        struct AccountPoint {
            QString label;
            double value = 0.0;
        };
        QList<AccountPoint> points;
        for (int i = 0; i < values.size(); ++i) {
            AccountPoint point;
            point.value = values.value(i);
            if (labels && i < labels->size())
                point.label = labels->value(i);
            points.append(point);
        }
        std::sort(points.begin(), points.end(), [](const AccountPoint& a, const AccountPoint& b) {
            return a.value > b.value;
        });
        while (points.size() > topAccountCount)
            points.removeLast();
        values.clear();
        if (labels)
            labels->clear();
        for (const AccountPoint& point : points) {
            values << point.value;
            if (labels)
                *labels << point.label;
        }
    };

    switch (id) {
    case M_SALES:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.months[i].sales;
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_SALES_RETURN:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.months[i].salesReturn;
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_PURCHASES:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
            const double fromSuppliers = d.suppliers[i].purchases;
            values << (fromSuppliers != 0.0 || !d.suppliers[i].supplierName.isEmpty() ? fromSuppliers : d.months[i].supplierPurchases);
        }
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_INVENTORY:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << (d.months[i].inventoryFirst + d.months[i].inventoryLast);
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_NET_SALES:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.netSales[i];
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_COGS:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.cogs[i];
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_PROFIT_MARGIN:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.profitMargin[i];
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_EXPENSES:
        if (hasAnyMonthlyExpenseAccounts(d)) {
            QStringList keys;
            QStringList outLabels;
            QVector<double> totals;
            for (int month = 0; month < 12; ++month) {
                if (!includeMonth(month))
                    continue;
                const QList<AccountItem> list = normalizedFixedExpenseAccountsForMonth(d.monthlyAccounts[month]);
                for (const auto& a : list) {
                    if (!accountMatchesFilter(a.type, accountFilter))
                        continue;
                    const QString label = expenseAccountDisplayName(a);
                    if (label.isEmpty())
                        continue;
                    const QString key = normalizedAccountKey(a);
                    int pos = keys.indexOf(key);
                    if (pos < 0) {
                        keys << key;
                        outLabels << label;
                        totals << 0.0;
                        pos = totals.size() - 1;
                    }
                    totals[pos] += a.amount;
                }
            }
            for (int i = 0; i < totals.size(); ++i) {
                values << totals.value(i);
                if (labels)
                    *labels << outLabels.value(i);
            }
        } else if (!d.accounts.isEmpty()) {
            for (const auto& a : d.accounts) {
                if (!accountMatchesFilter(a.type, accountFilter))
                    continue;
                values << a.amount;
            }
            if (labels) {
                for (const auto& a : d.accounts) {
                    if (!accountMatchesFilter(a.type, accountFilter))
                        continue;
                    const QString displayName = expenseAccountDisplayName(a);
                    *labels << (!a.code.trimmed().isEmpty() && fixedExpenseAccountIndexFromItem(a) < 0
                        ? (a.code.trimmed() + QStringLiteral(" - ") + displayName)
                        : displayName);
                }
            }
        } else {
            for (const auto& e : d.expenseSummary) values << e.total;
            if (labels) {
                for (const auto& e : d.expenseSummary)
                    *labels << e.account;
            }
        }
        break;
    case M_SUPPLIER_PAYMENTS:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
            const double fromSuppliers = d.suppliers[i].payments;
            values << (fromSuppliers != 0.0 || !d.suppliers[i].supplierName.isEmpty() ? fromSuppliers : d.months[i].supplierPayments);
        }
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_SUPPLIER_PREVIOUS_BALANCE:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
            double total = 0.0;
            for (const auto& e : d.supplierEntries[i]) total += e.previousBalance;
            values << total;
        }
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_SUPPLIER_TOTAL_DEBT:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
            double total = 0.0;
            for (const auto& e : d.supplierEntries[i]) total += (e.totalDebt > 0.0 ? e.totalDebt : (e.previousBalance + e.purchases));
            values << total;
        }
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_SUPPLIER_PAYMENT_PCT_PURCHASES:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
            double payments = 0.0, purchases = 0.0;
            for (const auto& e : d.supplierEntries[i]) { payments += e.payments; purchases += e.purchases; }
            values << (purchases > 0.0 ? (payments / purchases) * 100.0 : 0.0);
        }
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_SUPPLIER_PAYMENT_PCT_DEBT:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
            double payments = 0.0, debt = 0.0;
            for (const auto& e : d.supplierEntries[i]) { payments += e.payments; debt += (e.totalDebt > 0.0 ? e.totalDebt : (e.previousBalance + e.purchases)); }
            values << (debt > 0.0 ? (payments / debt) * 100.0 : 0.0);
        }
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_SUPPLIER_BALANCE:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
            double total = 0.0;
            for (const auto& e : d.supplierEntries[i]) total += ((e.totalDebt > 0.0 ? e.totalDebt : (e.previousBalance + e.purchases)) - e.payments);
            values << total;
        }
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_EXPENSE_AMOUNT:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
            values << (hasAnyMonthlyExpenseAccounts(d) ? monthlyExpenseAccountTotal(d, i) : d.months[i].expenseAmount);
        }
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_INVENTORY_OPENING:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.months[i].inventoryFirst;
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_INVENTORY_CLOSING:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.months[i].inventoryLast;
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) *labels << months.value(i);
        }
        break;
    case M_COGS_VS_PROFIT:
        break;
    case M_SUPPLIER_NAME:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
            values << 0.0;
        }
        if (labels) {
            const auto months = monthNames();
            for (int i = 0; i < 12; ++i) if (includeMonth(i)) {
                QString supplier = d.suppliers[i].supplierName.trimmed();
                if (supplier.isEmpty()) {
                    for (const auto& e : d.supplierEntries[i]) {
                        if (!e.name.trimmed().isEmpty()) { supplier = e.name.trimmed(); break; }
                    }
                }
                *labels << (supplier.isEmpty() ? months.value(i) : supplier);
            }
        }
        break;
    case M_COUNT:
        break;
    }
    applyTopAccountLimit();
    return values;
}



inline QColor metricColor(MetricId id)
{
    // High-contrast chart colors: avoid placing near-duplicate tones
    // such as light green and green in the same chart.
    switch (id) {
    case M_SALES:             return QColor("#1f77b4");
    case M_SALES_RETURN:      return QColor("#d62728");
    case M_PURCHASES:         return QColor("#9467bd");
    case M_EXPENSES:          return QColor("#17becf");
    case M_INVENTORY:         return QColor("#7f7f7f");
    case M_NET_SALES:         return QColor("#2ca02c");
    case M_COGS:              return QColor("#ff7f0e");
    case M_PROFIT_MARGIN:     return QColor("#e377c2");
    case M_SUPPLIER_PAYMENTS: return QColor("#8c564b");
    case M_SUPPLIER_PREVIOUS_BALANCE: return QColor("#2f4b7c");
    case M_SUPPLIER_TOTAL_DEBT: return QColor("#bc5090");
    case M_SUPPLIER_PAYMENT_PCT_PURCHASES: return QColor("#f0e442");
    case M_SUPPLIER_PAYMENT_PCT_DEBT: return QColor("#009e73");
    case M_SUPPLIER_BALANCE: return QColor("#e69f00");
    case M_EXPENSE_AMOUNT:    return QColor("#cc79a7");
    case M_INVENTORY_OPENING: return QColor("#56b4e9");
    case M_INVENTORY_CLOSING: return QColor("#6b7280");
    case M_COGS_VS_PROFIT:    return QColor("#1f77b4");
    case M_SUPPLIER_NAME:     return QColor("#665191");
    case M_COUNT:             break;
    }
    return QColor("#1f77b4");
}

inline QColor metricColorFromDisplayName(const QString& name)
{
    const QString key = name.trimmed();
    for (int i = 0; i < int(M_COUNT); ++i) {
        const MetricId id = static_cast<MetricId>(i);
        if (metricDisplayName(id).trimmed().compare(key, Qt::CaseInsensitive) == 0)
            return metricColor(id);
    }
    if (key.compare(QStringLiteral("Profit Margin"), Qt::CaseInsensitive) == 0 ||
        key.compare(QString::fromUtf8("هامش الربح"), Qt::CaseInsensitive) == 0)
        return metricColor(M_PROFIT_MARGIN);
    if (key.compare(tr_increasing_c5cd67().trimmed(), Qt::CaseInsensitive) == 0 ||
        key.compare(tr_increasing_faa4d2().trimmed(), Qt::CaseInsensitive) == 0)
        return metricColor(M_NET_SALES);
    if (key.compare(tr_decreasing_b4c279().trimmed(), Qt::CaseInsensitive) == 0 ||
        key.compare(tr_decreasing_d64136().trimmed(), Qt::CaseInsensitive) == 0)
        return QColor("#d62728");
    return QColor("#1f77b4");
}

inline QString comparisonTitle(MetricId a, MetricId b)
{
    return metricDisplayName(a) + tr_vs_6f0b2a() + metricDisplayName(b);
}

inline QString comparisonTitle(const QList<MetricId>& metrics)
{
    QStringList parts;
    for (MetricId id : metrics) {
        const QString name = metricDisplayName(id);
        if (!parts.contains(name))
            parts << name;
    }
    if (parts.isEmpty())
        return QString();
    return parts.join(QStringLiteral(", "));
}
