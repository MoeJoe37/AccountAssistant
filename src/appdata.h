#pragma once
#include <array>
#include <algorithm>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QColor>
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
    Payable = 0,
    Receivable = 1
};

enum class AccountTypeFilter {
    All = 0,
    Payable = 1,
    Receivable = 2
};

struct AccountItem {
    QString name;
    double  amount = 0.0;
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
    MetricLine
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

struct ChartRequest {
    ChartKind kind = ChartKind::Candle;
    MetricId metricA = M_SALES;
    MetricId metricB = M_SALES_RETURN;
    QList<MetricId> compareMetrics;  // Optional ordered list for 3+ metric comparisons.
    MetricId comparePieBaseMetric = M_COUNT;  // Optional base metric for ComparePie (100%).
    QString title;
    QString seriesA;
    QString seriesB;
    QList<int> months;   // Empty means all months
    AccountTypeFilter accountFilter = AccountTypeFilter::All;
    bool includeSummaryPoint = false;
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

// ─────────────────────────────────────────────────────────────────────────────
//  Full application state
// ─────────────────────────────────────────────────────────────────────────────
struct AppData {
    std::array<MonthData, 12> months{};
    std::array<ChartSel, M_COUNT> sel{};
    QList<AccountItem> accounts;
    std::array<SupplierMonthData, 12> suppliers{};
    std::array<QList<SupplierEntry>, 12> supplierEntries{};
    InventoryMode inventoryMode = InventoryMode::Periodic;

    // Chosen charts for the Results tab and PDF export
    QList<ChartRequest> chartRequests;
    QList<ChartRequest> hiddenChartRequests;
    QList<ResultFlowItem> resultFlowOrder;

    // Computed per-month
    std::array<double, 12> netSales{};
    std::array<double, 12> cogs{};
    std::array<double, 12> profitMargin{};

    // Grand totals (for summary bar)
    double totalNetSales = 0;
    double totalCOGS     = 0;
    double totalProfit   = 0;

    // Expense accounts aggregated & sorted by total (descending)
    QList<ExpenseSummary> expenseSummary;

    void calculate()
    {
        totalNetSales = totalCOGS = totalProfit = 0;
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

        // Build expense summary from the accounts tab, preserving the displayed order.
        // Legacy monthly expense fields are used only as a fallback for older data.
        expenseSummary.clear();
        if (!accounts.isEmpty()) {
            for (const auto& a : accounts) {
                const QString name = a.name.trimmed();
                if (!name.isEmpty() && a.amount > 0.0)
                    expenseSummary.append({name, a.amount});
            }
        } else {
            for (const auto& m : months) {
                const QString acc = m.expenseAccount.trimmed();
                if (!acc.isEmpty() && m.expenseAmount > 0.0)
                    expenseSummary.append({acc, m.expenseAmount});
            }
        }
    }
};


inline bool appDataHasUserEntries(const AppData& d)
{
    for (const auto& m : d.months) {
        if (m.sales != 0.0 || m.salesReturn != 0.0 || m.supplierPurchases != 0.0 ||
            m.supplierPayments != 0.0 || !m.supplierName.trimmed().isEmpty() ||
            !m.expenseAccount.trimmed().isEmpty() || m.expenseAmount != 0.0 ||
            m.inventoryFirst != 0.0 || m.inventoryLast != 0.0 || m.cogsInput != 0.0)
            return true;
    }

    if (!d.accounts.isEmpty() || !d.chartRequests.isEmpty() || !d.hiddenChartRequests.isEmpty() ||
        !d.resultFlowOrder.isEmpty())
        return true;

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

inline QString accountTypeDisplayName(AccountType type)
{
    switch (type) {
    case AccountType::Payable:   return tr_account_payable_003206();
    case AccountType::Receivable:return tr_account_receivable_59bf34();
    }
    return tr_account_payable_003206();
}

inline QString accountTypeFilterDisplayName(AccountTypeFilter type)
{
    switch (type) {
    case AccountTypeFilter::All:        return tr_all_b4d286();
    case AccountTypeFilter::Payable:    return tr_account_payable_003206();
    case AccountTypeFilter::Receivable: return tr_account_receivable_59bf34();
    }
    return tr_all_b4d286();
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
    case M_SUPPLIER_PREVIOUS_BALANCE:return T("Supplier previous balance", "الرصيد السابق للمورد");
    case M_SUPPLIER_TOTAL_DEBT:return T("Supplier total debt", "إجمالي دين المورد");
    case M_SUPPLIER_PAYMENT_PCT_PURCHASES:return T("Supplier payment % of purchases", "نسبة دفع المورد من المشتريات");
    case M_SUPPLIER_PAYMENT_PCT_DEBT:return T("Supplier payment % of debt", "نسبة دفع المورد من الدين");
    case M_SUPPLIER_BALANCE:return T("Supplier balance", "رصيد المورد");
    case M_EXPENSE_AMOUNT:   return tr_expense_amount_1ad3d5();
    case M_INVENTORY_OPENING:return tr_inventory_opening_ccde20();
    case M_INVENTORY_CLOSING:return tr_inventory_closing_d69943();
    case M_COGS_VS_PROFIT:   return tr_cogs_vs_profit_margin_fd48e9();
    case M_COUNT:            break;
    }
    return tr_unknown_0240b2();
}

inline bool metricUsesMonthlySeries(MetricId id)
{
    return id != M_EXPENSES && id != M_COGS_VS_PROFIT;
}

inline QList<double> metricSeriesValues(const AppData& d, MetricId id, QStringList* labels = nullptr, const QList<int>* monthFilter = nullptr, AccountTypeFilter accountFilter = AccountTypeFilter::All)
{
    if (labels)
        labels->clear();

    const auto includeMonth = [&](int idx) {
        return !monthFilter || monthFilter->isEmpty() || monthFilter->contains(idx);
    };

    QList<double> values;
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
        if (!d.accounts.isEmpty()) {
            for (const auto& a : d.accounts) {
                if (accountFilter == AccountTypeFilter::Payable && a.type != AccountType::Payable)
                    continue;
                if (accountFilter == AccountTypeFilter::Receivable && a.type != AccountType::Receivable)
                    continue;
                values << a.amount;
            }
            if (labels) {
                for (const auto& a : d.accounts) {
                    if (accountFilter == AccountTypeFilter::Payable && a.type != AccountType::Payable)
                        continue;
                    if (accountFilter == AccountTypeFilter::Receivable && a.type != AccountType::Receivable)
                        continue;
                    *labels << a.name;
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
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.months[i].expenseAmount;
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
    case M_COUNT:
        break;
    }
    return values;
}



inline QColor metricColor(MetricId id)
{
    switch (id) {
    case M_SALES:             return QColor("#4f86f7");
    case M_SALES_RETURN:      return QColor("#e05c6a");
    case M_PURCHASES:         return QColor("#9b6cf9");
    case M_EXPENSES:          return QColor("#62c4e3");
    case M_INVENTORY:         return QColor("#8f97b4");
    case M_NET_SALES:         return QColor("#3ecf8e");
    case M_COGS:              return QColor("#f0a500");
    case M_PROFIT_MARGIN:     return QColor("#14b8a6");
    case M_SUPPLIER_PAYMENTS: return QColor("#ff9f43");
    case M_SUPPLIER_PREVIOUS_BALANCE: return QColor("#7c83fd");
    case M_SUPPLIER_TOTAL_DEBT: return QColor("#ef476f");
    case M_SUPPLIER_PAYMENT_PCT_PURCHASES: return QColor("#06d6a0");
    case M_SUPPLIER_PAYMENT_PCT_DEBT: return QColor("#118ab2");
    case M_SUPPLIER_BALANCE: return QColor("#ffd166");
    case M_EXPENSE_AMOUNT:    return QColor("#fd79a8");
    case M_INVENTORY_OPENING: return QColor("#5b8def");
    case M_INVENTORY_CLOSING: return QColor("#8f97b4");
    case M_COGS_VS_PROFIT:    return QColor("#4f86f7");
    case M_COUNT:             break;
    }
    return QColor("#4f86f7");
}

inline QColor metricColorFromDisplayName(const QString& name)
{
    const QString key = name.trimmed();
    for (int i = 0; i < int(M_COUNT); ++i) {
        const MetricId id = static_cast<MetricId>(i);
        if (metricDisplayName(id).trimmed().compare(key, Qt::CaseInsensitive) == 0)
            return metricColor(id);
    }
    if (key.compare(tr_increasing_c5cd67().trimmed(), Qt::CaseInsensitive) == 0 ||
        key.compare(tr_increasing_faa4d2().trimmed(), Qt::CaseInsensitive) == 0)
        return metricColor(M_NET_SALES);
    if (key.compare(tr_decreasing_b4c279().trimmed(), Qt::CaseInsensitive) == 0 ||
        key.compare(tr_decreasing_d64136().trimmed(), Qt::CaseInsensitive) == 0)
        return metricColor(M_NET_SALES).darker(135);
    return QColor("#4f86f7");
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
