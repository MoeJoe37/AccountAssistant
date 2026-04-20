#include "chartswidget.h"
#include "translations.h"

#include <QChart>
#include <QAbstractSeries>
#include <QPieSeries>
#include <QPieSlice>
#include <QLegendMarker>
#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QFont>
#include <QColor>
#include <QList>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCategoryAxis>
#include <QTimer>

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
    explicit SafeChartView(QChart* chart) : QChartView(chart)
    {
        setRenderHint(QPainter::Antialiasing);
        setDragMode(QGraphicsView::NoDrag);
        setInteractive(false);
        setRubberBand(QChartView::NoRubberBand);
        setMouseTracking(false);
    }

protected:
    void wheelEvent(QWheelEvent* e) override
    {
        if (e) e->accept();
    }

    void mousePressEvent(QMouseEvent* e) override
    {
        if (e && e->button() == Qt::RightButton) {
            e->accept();
            return;
        }
        if (e) e->accept();
    }

    void mouseReleaseEvent(QMouseEvent* e) override
    {
        if (e) e->accept();
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if (e) e->accept();
    }
};

static QChartView* makeSafeView(QChart* chart)
{
    return new SafeChartView(chart);
}

static void applyLegendMarkerColorsLater(QChart* chart,
                                         QAbstractSeries* series,
                                         const QStringList& labels,
                                         const QList<QColor>& colors)
{
    if (!chart || !series) return;
    QTimer::singleShot(0, chart, [chart, series, labels, colors]() {
        const auto markers = chart->legend()->markers(series);
        for (int i = 0; i < markers.size() && i < colors.size(); ++i) {
            if (!markers[i]) continue;
            if (i < labels.size() && !labels[i].isEmpty())
                markers[i]->setLabel(labels[i]);
            markers[i]->setBrush(QBrush(colors[i]));
            markers[i]->setPen(QPen(colors[i].darker(140)));
        }
    });
}

static QWidget* makeLegendWidget(QWidget* parent, const QStringList& labels, const QList<QColor>& colors)
{
    if (labels.isEmpty() || colors.isEmpty())
        return nullptr;

    auto* wrap = new QWidget(parent);
    auto* grid = new QGridLayout(wrap);
    grid->setContentsMargins(8, 4, 8, 8);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(6);

    const int cols = labels.size() > 4 ? 2 : 3;
    for (int i = 0; i < labels.size() && i < colors.size(); ++i) {
        auto* item = new QWidget(wrap);
        auto* hl = new QHBoxLayout(item);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(6);
        auto* swatch = new QFrame(item);
        swatch->setFixedSize(10, 10);
        swatch->setAutoFillBackground(true);
        swatch->setStyleSheet(QString("background-color:%1; border:1px solid #d4dbea; border-radius:2px;").arg(colors[i].name()));
        auto* lab = new QLabel(labels[i], item);
        lab->setStyleSheet("background:transparent;color:#ffffff;font-size:11px;");
        hl->addWidget(swatch);
        hl->addWidget(lab);
        hl->addStretch();
        grid->addWidget(item, i / cols, i % cols);
    }
    return wrap;
}

static QWidget* wrapChartWithLegend(QWidget* parent, QChartView* view,
                                    const QStringList& labels, const QList<QColor>& colors)
{
    auto* wrap = new QWidget(parent);
    auto* vl = new QVBoxLayout(wrap);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(4);
    view->setParent(wrap);
    vl->addWidget(view, 1);
    if (auto* legend = makeLegendWidget(wrap, labels, colors))
        vl->addWidget(legend, 0);
    return wrap;
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
        if (!v) continue;
        QWidget* w = v->parentWidget();
        if (!w) w = v;
        m_layout->removeWidget(w);
        w->deleteLater();
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
        addPieChart(tr_sales_4af850(), months, monthVals(&MonthData::sales));
    if (d.sel[M_SALES].candle)
        addCandleChart(tr_sales_4af850(), months, monthVals(&MonthData::sales));
    if (d.sel[M_SALES].line)
        addLineChart(tr_sales_4af850(), months, monthVals(&MonthData::sales));

    // ── Sales Return ──────────────────────────────────────────────────────
    if (d.sel[M_SALES_RETURN].pie)
        addPieChart(tr_sales_return_7b335a(),
                    months, monthVals(&MonthData::salesReturn));
    if (d.sel[M_SALES_RETURN].candle)
        addCandleChart(tr_sales_return_7b335a(),
                       months, monthVals(&MonthData::salesReturn));
    if (d.sel[M_SALES_RETURN].line)
        addLineChart(tr_sales_return_e520e9(),
                     months, monthVals(&MonthData::salesReturn));

    // ── Purchases ─────────────────────────────────────────────────────────
    if (d.sel[M_PURCHASES].pie) {
        QList<double> combined;
        for (auto& m : d.months) combined << (m.supplierPurchases + m.supplierPayments);
        addPieChart(tr_purchases_988898(), months, combined);
    }
    if (d.sel[M_PURCHASES].candle)
        addCandleChart(tr_purchases_988898(),
                       months, monthVals(&MonthData::supplierPurchases));
    if (d.sel[M_PURCHASES].line)
        addLineChart(tr_purchases_16236c(),
                     months, monthVals(&MonthData::supplierPurchases));

    // ── Expenses ──────────────────────────────────────────────────────────
    if (d.sel[M_EXPENSES].pie)
        addPieChart(tr_expenses_ed49c9(),
                    months, monthVals(&MonthData::expenseAmount));
    if (d.sel[M_EXPENSES].candle)
        addCandleChart(tr_expenses_ed49c9(),
                       months, monthVals(&MonthData::expenseAmount));
    if (d.sel[M_EXPENSES].line)
        addLineChart(tr_expenses_5a0c3c(),
                     months, monthVals(&MonthData::expenseAmount));

    // ── Inventory ─────────────────────────────────────────────────────────
    if (d.sel[M_INVENTORY].pie) {
        QList<double> combined;
        for (auto& m : d.months) combined << (m.inventoryFirst + m.inventoryLast);
        addPieChart(tr_inventory_d636e9(), months, combined);
    }
    if (d.sel[M_INVENTORY].candle)
        addCandleChart(tr_inventory_d636e9(),
                       months, monthVals(&MonthData::inventoryFirst));
    if (d.sel[M_INVENTORY].line)
        addLineChart(tr_inventory_18734c(),
                     months, monthVals(&MonthData::inventoryFirst));

    // ── Net Sales ─────────────────────────────────────────────────────────
    if (d.sel[M_NET_SALES].pie)
        addPieChart(tr_net_sales_ae3003(),
                    months, arr12(d.netSales));
    if (d.sel[M_NET_SALES].candle)
        addCandleChart(tr_net_sales_ae3003(),
                       months, arr12(d.netSales));
    if (d.sel[M_NET_SALES].line)
        addLineChart(tr_net_sales_23a2f1(),
                     months, arr12(d.netSales));

    // ── COGS ──────────────────────────────────────────────────────────────
    if (d.sel[M_COGS].pie)
        addPieChart(tr_cost_of_goods_sold_31b73d(),
                    months, arr12(d.cogs));
    if (d.sel[M_COGS].candle)
        addCandleChart(tr_cost_of_goods_sold_31b73d(),
                       months, arr12(d.cogs));
    if (d.sel[M_COGS].line)
        addLineChart(tr_cost_of_goods_sold_6e7684(),
                     months, arr12(d.cogs));

    // ── Profit Margin ─────────────────────────────────────────────────────
    if (d.sel[M_PROFIT_MARGIN].pie)
        addPieChart(tr_profit_margin_ff57d3(),
                    months, arr12(d.profitMargin));
    if (d.sel[M_PROFIT_MARGIN].candle)
        addCandleChart(tr_profit_margin_ff57d3(),
                       months, arr12(d.profitMargin));
    if (d.sel[M_PROFIT_MARGIN].line)
        addLineChart(tr_profit_margin_ec3b22(),
                     months, arr12(d.profitMargin));
}

// ─────────────────────────────────────────────────────────────────────────────
void ChartsWidget::addPieChart(const QString& title,
                                const QStringList& labels,
                                const QList<double>& values)
{
    auto* cv = makePie(title, labels, values);
    m_views << cv;
    QList<QColor> colors;
    for (int i = 0; i < labels.size() && i < values.size(); ++i)
        colors << kPalette[i % kPalette.size()];
    auto* wrap = wrapChartWithLegend(m_container, cv, labels, colors);
    m_layout->insertWidget(m_layout->count()-1, wrap);
}

void ChartsWidget::addCandleChart(const QString& title,
                                   const QStringList& labels,
                                   const QList<double>& values)
{
    auto* cv = makeCandle(title, labels, values);
    m_views << cv;
    auto* wrap = wrapChartWithLegend(m_container, cv,
                                     QStringList{tr_increasing_faa4d2(), tr_decreasing_d64136()},
                                     QList<QColor>{QColor("#59A14F"), QColor("#E15759")});
    m_layout->insertWidget(m_layout->count()-1, wrap);
}

void ChartsWidget::addLineChart(const QString& title,
                                const QStringList& labels,
                                const QList<double>& values)
{
    auto* cv = makeLine(title, labels, values);
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

    QList<QColor> sliceColors;
    for (int i = 0; i < labels.size() && i < values.size(); ++i) {
        double val = qAbs(values[i]);
        if (val == 0) continue;
        const QColor c = kPalette[i % kPalette.size()];
        auto* slice = series->append(labels[i], val);
        slice->setColor(c);
        sliceColors << c;
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelArmLengthFactor(0.18);
        slice->setLabelColor(Qt::white);
        slice->setLabel(QString::number((val / total) * 100.0, 'f', 1) + QStringLiteral("%"));
    }

    series->setPieSize(0.72);
    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setTitle(title);
    chart->setTitleFont(QFont("Segoe UI", 11, QFont::Bold));
    chart->setBackgroundBrush(QBrush(QColor("#1e2235")));
    chart->setTitleBrush(QBrush(Qt::white));
    chart->legend()->setVisible(false);
    chart->setMargins(QMargins(2, 2, 2, 20));
    chart->setAnimationOptions(QChart::AllAnimations);

    applyLegendMarkerColorsLater(chart, series, labels, sliceColors);

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
    legendInc->setName(tr_increasing_c5cd67());
    legendInc->setColor(QColor("#59A14F"));
    auto* legendDec = new QLineSeries;
    legendDec->setName(tr_decreasing_b4c279());
    legendDec->setColor(QColor("#E15759"));
    chart->addSeries(legendInc);
    chart->addSeries(legendDec);
    legendInc->attachAxis(axisX);
    legendInc->attachAxis(axisY);
    legendDec->attachAxis(axisX);
    legendDec->attachAxis(axisY);

    chart->legend()->setVisible(false);
    applyLegendMarkerColorsLater(chart, legendInc, QStringList{tr_increasing_c5cd67()}, QList<QColor>{QColor("#59A14F")});
    applyLegendMarkerColorsLater(chart, legendDec, QStringList{tr_decreasing_b4c279()}, QList<QColor>{QColor("#E15759")});
    chart->setMargins(QMargins(2, 2, 2, 20));
    chart->setAnimationOptions(QChart::AllAnimations);

    auto* view = makeSafeView(chart);
    view->setFixedSize(420, 360);
    view->setObjectName("chartView");
    return view;
}


QChartView* ChartsWidget::makeLine(const QString& title,
                                   const QStringList& labels,
                                   const QList<double>& values)
{
    auto* line = new QLineSeries;
    line->setName(title);
    line->setColor(QColor("#4f86f7"));
    for (int i = 0; i < values.size(); ++i)
        line->append(i, values[i]);

    auto* chart = new QChart;
    chart->addSeries(line);
    chart->setTitle(title);
    chart->setTitleFont(QFont("Segoe UI", 11, QFont::Bold));
    chart->setBackgroundBrush(QBrush(QColor("#1e2235")));
    chart->setTitleBrush(QBrush(Qt::white));
    chart->legend()->setVisible(false);

    auto* axisX = new QCategoryAxis;
    for (int i = 0; i < labels.size(); ++i)
        axisX->append(labels[i], i);
    axisX->setRange(0, qMax(0, labels.size() - 1));
    axisX->setLabelsColor(Qt::white);
    axisX->setLabelsAngle(-45);
    axisX->setGridLineColor(QColor("#3a3f55"));
    chart->addAxis(axisX, Qt::AlignBottom);
    line->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setLabelsColor(Qt::white);
    axisY->setGridLineColor(QColor("#3a3f55"));
    double maxV = 0.0;
    for (double v : values) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    axisY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axisY, Qt::AlignLeft);
    line->attachAxis(axisY);

    auto* view = makeSafeView(chart);
    view->setFixedSize(420, 360);
    view->setObjectName("chartView");
    return view;
}
