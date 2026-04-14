#pragma once
#include <array>
#include <algorithm>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include "translations.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Per-month data (exactly what the user enters in the table)
// ─────────────────────────────────────────────────────────────────────────────
struct MonthData {
    double sales             = 0.0;
    double salesReturn       = 0.0;
    double supplierPurchases = 0.0;
    double supplierPayments  = 0.0;
    QString expenseAccount;
    double expenseAmount     = 0.0;
    double inventoryFirst    = 0.0;
    double inventoryLast     = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Expense/account entry independent from months
// ─────────────────────────────────────────────────────────────────────────────
struct AccountItem {
    QString name;
    double  amount = 0.0;
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

struct ChartRequest {
    ChartKind kind = ChartKind::Candle;
    MetricId metricA = M_SALES;
    MetricId metricB = M_SALES_RETURN;
    QString title;
    QString seriesA;
    QString seriesB;
    QList<int> months;   // Empty means all months
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

// ─────────────────────────────────────────────────────────────────────────────
//  Full application state
// ─────────────────────────────────────────────────────────────────────────────
struct AppData {
    std::array<MonthData, 12> months{};
    std::array<ChartSel, M_COUNT> sel{};
    QList<AccountItem> accounts;

    // Chosen charts for the Results tab and PDF export
    QList<ChartRequest> chartRequests;
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
            cogs[i]         = m.inventoryFirst + m.supplierPurchases - m.inventoryLast;
            profitMargin[i] = netSales[i] - cogs[i];

            totalNetSales += netSales[i];
            totalCOGS     += cogs[i];
            totalProfit   += profitMargin[i];
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

inline QString metricDisplayName(MetricId id)
{
    switch (id) {
    case M_SALES:            return T("Sales", "\u0627\u0644\u0645\u0628\u064A\u0639\u0627\u062A");
    case M_SALES_RETURN:     return T("Sales Return", "\u0645\u0631\u062A\u062C\u0639\u0627\u062A \u0627\u0644\u0645\u0628\u064A\u0639\u0627\u062A");
    case M_PURCHASES:        return T("Purchases", "\u0627\u0644\u0645\u0634\u062A\u0631\u064A\u0627\u062A");
    case M_EXPENSES:         return T("Expenses", "\u0627\u0644\u0645\u0635\u0631\u0648\u0641\u0627\u062A");
    case M_INVENTORY:        return T("Inventory", "\u0627\u0644\u0645\u062E\u0632\u0648\u0646");
    case M_NET_SALES:        return T("Net Sales", "\u0635\u0627\u0641\u064A \u0627\u0644\u0645\u0628\u064A\u0639\u0627\u062A");
    case M_COGS:             return T("Cost of Goods Sold", "\u062A\u0643\u0644\u0641\u0629 \u0627\u0644\u0628\u0636\u0627\u0639\u0629");
    case M_PROFIT_MARGIN:    return T("Profit Margin", "\u0647\u0627\u0645\u0634 \u0627\u0644\u0631\u0628\u062D");
    case M_SUPPLIER_PAYMENTS:return T("Supplier Payments", "\u062F\u0641\u0639\u0627\u062A \u0627\u0644\u0645\u0648\u0631\u062F\u064A\u0646");
    case M_EXPENSE_AMOUNT:   return T("Expense Amount", "\u0645\u0628\u0644\u063A \u0627\u0644\u0645\u0635\u0631\u0648\u0641");
    case M_INVENTORY_OPENING:return T("Inventory Opening", "\u0627\u0644\u0645\u062E\u0632\u0648\u0646 \u0627\u0644\u0627\u0641\u062A\u062A\u0627\u062D\u064A");
    case M_INVENTORY_CLOSING:return T("Inventory Closing", "\u0627\u0644\u0645\u062E\u0632\u0648\u0646 \u0627\u0644\u062E\u062A\u0627\u0645\u064A");
    case M_COGS_VS_PROFIT:   return T("COGS vs Profit Margin", "\u062A\u0643\u0644\u0641\u0629 \u0645\u0642\u0627\u0628\u0644 \u0627\u0644\u0631\u0628\u062D");
    case M_COUNT:            break;
    }
    return T("Unknown", "\u063A\u064A\u0631 \u0645\u0639\u0631\u0648\u0641");
}

inline bool metricUsesMonthlySeries(MetricId id)
{
    return id != M_EXPENSES && id != M_COGS_VS_PROFIT;
}

inline QList<double> metricSeriesValues(const AppData& d, MetricId id, QStringList* labels = nullptr, const QList<int>* monthFilter = nullptr)
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
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.months[i].supplierPurchases;
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
        for (const auto& e : d.expenseSummary) values << e.total;
        if (labels) {
            for (const auto& e : d.expenseSummary)
                *labels << e.account;
        }
        break;
    case M_SUPPLIER_PAYMENTS:
        for (int i = 0; i < 12; ++i) if (includeMonth(i)) values << d.months[i].supplierPayments;
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

inline QString comparisonTitle(MetricId a, MetricId b)
{
    return metricDisplayName(a) + QStringLiteral(" vs ") + metricDisplayName(b);
}
