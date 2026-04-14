#include "chartswidget.h"
#include "translations.h"

#include <QChart>
#include <QPieSeries>
#include <QPieSlice>
#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <QColor>
#include <QList>
#include <QPainter>
#include <QMouseEvent>

// ─────────────────────────────────────────────────────────────────────────────
//  Palette
// ─────────────────────────────────────────────────────────────────────────────
static const QList<QColor> kPalette = {
    "#4E79A7","#F28E2B","#E15759","#76B7B2",
    "#59A14F","#EDC948","#B07AA1","#FF9DA7",
    "#9C755F","#BAB0AC","#D4A6C8","#86BCB6"
};

// ─────────────────────────────────────────────────────────────────────────────
class SafeChartView : public QChartView
{
public:
    using QChartView::QChartView;
protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e && e->button() == Qt::RightButton) {
            e->accept();
            return;
        }
        QChartView::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        if (e && e->button() == Qt::RightButton) {
            e->accept();
            return;
        }
        QChartView::mouseReleaseEvent(e);
    }
};

static QChartView* makeSafeView(QChart* chart)
{
    auto* view = new SafeChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    return view;
}

ChartsWidget::ChartsWidget(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);

    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setFrameShape(QFrame::NoFrame);

    m_container = new QWidget;
    m_container->setObjectName("chartsContainer");
    m_layout = new QHBoxLayout(m_container);
    m_layout->setSpacing(16);
    m_layout->setContentsMargins(12,12,12,12);
    m_layout->addStretch();

    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll);
}

// ─────────────────────────────────────────────────────────────────────────────
void ChartsWidget::clear()
{
    for (auto* v : m_views) {
        m_layout->removeWidget(v);
        v->deleteLater();
    }
    m_views.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
void ChartsWidget::buildCharts(const AppData& d)
{
    clear();

    QStringList months = monthNames();

    // Helper: extract per-month values from MonthData field
    auto monthVals = [&](auto field) {
        QList<double> v;
        for (auto& m : d.months) v << (m.*field);
        return v;
    };

    auto arr12 = [&](const std::array<double,12>& a) {
        QList<double> v;
        for (double x : a) v << x;
        return v;
    };

    // ── Sales ─────────────────────────────────────────────────────────────
    if (d.sel[M_SALES].pie)
        addPieChart(T("Sales","المبيعات"), months, monthVals(&MonthData::sales));
    if (d.sel[M_SALES].candle)
        addCandleChart(T("Sales","المبيعات"), months, monthVals(&MonthData::sales));

    // ── Sales Return ──────────────────────────────────────────────────────
    if (d.sel[M_SALES_RETURN].pie)
        addPieChart(T("Sales Return","\u0645\u0631\u062a\u062c\u0639\u0627\u062a"),
                    months, monthVals(&MonthData::salesReturn));
    if (d.sel[M_SALES_RETURN].candle)
        addCandleChart(T("Sales Return","\u0645\u0631\u062a\u062c\u0639\u0627\u062a"),
                       months, monthVals(&MonthData::salesReturn));

    // ── Purchases ─────────────────────────────────────────────────────────
    if (d.sel[M_PURCHASES].pie) {
        QList<double> combined;
        for (auto& m : d.months) combined << (m.supplierPurchases + m.supplierPayments);
        addPieChart(T("Purchases","\u0645\u0634\u062a\u0631\u064a\u0627\u062a"), months, combined);
    }
    if (d.sel[M_PURCHASES].candle)
        addCandleChart(T("Purchases","\u0645\u0634\u062a\u0631\u064a\u0627\u062a"),
                       months, monthVals(&MonthData::supplierPurchases));

    // ── Expenses ──────────────────────────────────────────────────────────
    if (d.sel[M_EXPENSES].pie)
        addPieChart(T("Expenses","\u0645\u0635\u0631\u0648\u0641\u0627\u062a"),
                    months, monthVals(&MonthData::expenseAmount));
    if (d.sel[M_EXPENSES].candle)
        addCandleChart(T("Expenses","\u0645\u0635\u0631\u0648\u0641\u0627\u062a"),
                       months, monthVals(&MonthData::expenseAmount));

    // ── Inventory ─────────────────────────────────────────────────────────
    if (d.sel[M_INVENTORY].pie) {
        QList<double> combined;
        for (auto& m : d.months) combined << (m.inventoryFirst + m.inventoryLast);
        addPieChart(T("Inventory","\u0645\u062e\u0632\u0648\u0646"), months, combined);
    }
    if (d.sel[M_INVENTORY].candle)
        addCandleChart(T("Inventory","\u0645\u062e\u0632\u0648\u0646"),
                       months, monthVals(&MonthData::inventoryFirst));

    // ── Net Sales ─────────────────────────────────────────────────────────
    if (d.sel[M_NET_SALES].pie)
        addPieChart(T("Net Sales","\u0635\u0627\u0641\u064a \u0627\u0644\u0645\u0628\u064a\u0639\u0627\u062a"),
                    months, arr12(d.netSales));
    if (d.sel[M_NET_SALES].candle)
        addCandleChart(T("Net Sales","\u0635\u0627\u0641\u064a \u0627\u0644\u0645\u0628\u064a\u0639\u0627\u062a"),
                       months, arr12(d.netSales));

    // ── COGS ──────────────────────────────────────────────────────────────
    if (d.sel[M_COGS].pie)
        addPieChart(T("Cost of Goods Sold","\u062a\u0643\u0644\u0641\u0629 \u0627\u0644\u0628\u0636\u0627\u0639\u0629"),
                    months, arr12(d.cogs));
    if (d.sel[M_COGS].candle)
        addCandleChart(T("Cost of Goods Sold","\u062a\u0643\u0644\u0641\u0629 \u0627\u0644\u0628\u0636\u0627\u0639\u0629"),
                       months, arr12(d.cogs));

    // ── Profit Margin ─────────────────────────────────────────────────────
    if (d.sel[M_PROFIT_MARGIN].pie)
        addPieChart(T("Profit Margin","\u0647\u0627\u0645\u0634 \u0627\u0644\u0631\u0628\u062d"),
                    months, arr12(d.profitMargin));
    if (d.sel[M_PROFIT_MARGIN].candle)
        addCandleChart(T("Profit Margin","\u0647\u0627\u0645\u0634 \u0627\u0644\u0631\u0628\u062d"),
                       months, arr12(d.profitMargin));
}

// ─────────────────────────────────────────────────────────────────────────────
void ChartsWidget::addPieChart(const QString& title,
                                const QStringList& labels,
                                const QList<double>& values)
{
    auto* cv = makePie(title, labels, values);
    m_views << cv;
    m_layout->insertWidget(m_layout->count()-1, cv);
}

void ChartsWidget::addCandleChart(const QString& title,
                                   const QStringList& labels,
                                   const QList<double>& values)
{
    auto* cv = makeCandle(title, labels, values);
    m_views << cv;
    m_layout->insertWidget(m_layout->count()-1, cv);
}

// ─────────────────────────────────────────────────────────────────────────────
QChartView* ChartsWidget::makePie(const QString& title,
                                   const QStringList& labels,
                                   const QList<double>& values)
{
    auto* series = new QPieSeries;
    double total = 0;
    for (double v : values) total += qAbs(v);
    if (total == 0) total = 1;

    for (int i = 0; i < labels.size() && i < values.size(); ++i) {
        double val = qAbs(values[i]);
        if (val == 0) continue;
        auto* slice = series->append(labels[i], val);
        slice->setColor(kPalette[i % kPalette.size()]);
        slice->setLabelVisible(true);
        slice->setLabel(QString("%1%").arg(val / total * 100.0, 0, 'f', 1));
        slice->setLabelPosition(QPieSlice::LabelInsideTangential);
        slice->setLabelColor(Qt::white);
    }

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(title);
    chart->setTitleFont(QFont("Segoe UI", 11, QFont::Bold));
    chart->setBackgroundBrush(QBrush(QColor("#1e2235")));
    chart->setTitleBrush(QBrush(Qt::white));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setFont(QFont("Segoe UI", 9));
    chart->legend()->setLabelColor(Qt::white);
    chart->legend()->setBackgroundVisible(false);
    chart->setAnimationOptions(QChart::AllAnimations);

    auto* view = makeSafeView(chart);
    view->setFixedSize(380, 320);
    view->setObjectName("chartView");
    return view;
}

// ─────────────────────────────────────────────────────────────────────────────
QChartView* ChartsWidget::makeCandle(const QString& title,
                                      const QStringList& labels,
                                      const QList<double>& values)
{
    auto* series = new QCandlestickSeries;
    series->setName(title);
    series->setIncreasingColor(QColor("#59A14F"));
    series->setDecreasingColor(QColor("#E15759"));

    for (int i = 0; i < values.size(); ++i) {
        double cur = values[i];
        double open  = 0.0;
        double close = cur;
        double high  = qMax(open, close);
        double low   = qMin(open, close);
        if (high == low) { high += 0.01; }  // avoid degenerate candle
        auto* cs = new QCandlestickSet(open, high, low, close,
                                       static_cast<qreal>(i));
        series->append(cs);
    }

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(title);
    chart->setTitleFont(QFont("Segoe UI", 11, QFont::Bold));
    chart->setBackgroundBrush(QBrush(QColor("#1e2235")));
    chart->setTitleBrush(QBrush(Qt::white));

    // Category axis
    auto* axisX = new QBarCategoryAxis;
    axisX->append(labels);
    axisX->setLabelsColor(Qt::white);
    axisX->setLabelsAngle(-45);
    axisX->setGridLineColor(QColor("#3a3f55"));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setLabelsColor(Qt::white);
    axisY->setGridLineColor(QColor("#3a3f55"));
    double maxV = 0.0;
    for (double v : values) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    axisY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Legend: add two invisible dummy series so the colour key is visible
    auto* legendInc = new QLineSeries;
    legendInc->setName(T("↗ Increasing", "↗ ارتفاع"));
    legendInc->setColor(QColor("#59A14F"));
    auto* legendDec = new QLineSeries;
    legendDec->setName(T("↘ Decreasing", "↘ انخفاض"));
    legendDec->setColor(QColor("#E15759"));
    chart->addSeries(legendInc);
    chart->addSeries(legendDec);
    legendInc->attachAxis(axisX);
    legendInc->attachAxis(axisY);
    legendDec->attachAxis(axisX);
    legendDec->attachAxis(axisY);

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setFont(QFont("Segoe UI", 9));
    chart->legend()->setLabelColor(Qt::white);
    chart->legend()->setBackgroundVisible(false);
    chart->setAnimationOptions(QChart::AllAnimations);

    auto* view = makeSafeView(chart);
    view->setFixedSize(420, 360);
    view->setObjectName("chartView");
    return view;
}
