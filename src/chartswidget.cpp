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
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  Palette
// ─────────────────────────────────────────────────────────────────────────────
static const QList<QColor> kPalette = {
    "#1f77b4", "#ff7f0e", "#d62728", "#9467bd",
    "#8c564b", "#e377c2", "#17becf", "#bcbd22",
    "#7f7f7f", "#2f4b7c", "#665191", "#009e73"
};

static QColor paletteColor(int index)
{
    if (kPalette.isEmpty())
        return QColor("#1f77b4");
    return kPalette[qAbs(index) % kPalette.size()];
}

static QString compactMoneyText(double value)
{
    const double absValue = qAbs(value);
    const char* suffix = "";
    double divisor = 1.0;
    if (absValue >= 1000000000.0) { suffix = "B"; divisor = 1000000000.0; }
    else if (absValue >= 1000000.0) { suffix = "M"; divisor = 1000000.0; }
    else if (absValue >= 1000.0) { suffix = "K"; divisor = 1000.0; }

    if (divisor <= 1.0)
        return QString("%L1").arg(qRound64(value));

    const double scaled = std::floor((absValue / divisor) * 10.0) / 10.0;
    QString text = QString::number(scaled, 'f', 1);
    if (text.endsWith(QStringLiteral(".0")))
        text.chop(2);
    if (value < 0.0)
        text.prepend(QChar('-'));
    return text + QString::fromLatin1(suffix);
}

static void setCompactMoneyAxisRange(QCategoryAxis* axis, double maxAbs)
{
    if (!axis) return;
    if (maxAbs < 0.001) maxAbs = 1.0;
    const double upper = maxAbs * 1.1;
    axis->setRange(0.0, upper);
    const int tickCount = 5;
    for (int i = 0; i < tickCount; ++i) {
        const double value = upper * i / double(tickCount - 1);
        axis->append(i == 0 ? QStringLiteral("0") : compactMoneyText(value), value);
    }
}

static QColor contrastDecreaseColor(const QColor& base)
{
    const int hue = base.hsvHue();
    if (hue >= 0 && (hue < 35 || hue > 335))
        return QColor("#1f77b4");
    return QColor("#d62728");
}

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

    const QString monthSeparator = QStringLiteral(" — ");
    bool allHaveMonthPrefix = true;
    QStringList orderedMonths;
    QMap<QString, QList<int>> monthRows;

    for (int i = 0; i < labels.size() && i < colors.size(); ++i) {
        const int sep = labels[i].indexOf(monthSeparator);
        if (sep <= 0) {
            allHaveMonthPrefix = false;
            break;
        }
        const QString month = labels[i].left(sep).trimmed();
        if (!monthRows.contains(month))
            orderedMonths << month;
        monthRows[month] << i;
    }

    if (allHaveMonthPrefix && orderedMonths.size() > 1) {
        auto* wrap = new QWidget(parent);
        wrap->setLayoutDirection(Qt::LeftToRight);
        auto* hl = new QHBoxLayout(wrap);
        hl->setContentsMargins(8, 4, 8, 8);
        hl->setSpacing(18);

        for (const QString& month : orderedMonths) {
            auto* colWrap = new QFrame(wrap);
            colWrap->setLayoutDirection(Qt::LeftToRight);
            colWrap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            auto* col = new QVBoxLayout(colWrap);
            col->setContentsMargins(0, 0, 0, 0);
            col->setSpacing(4);

            auto* monthLab = new QLabel(month, colWrap);
            monthLab->setAlignment(Qt::AlignCenter);
            monthLab->setStyleSheet("background:transparent;color:#ffffff;font-size:11px;font-weight:700;");
            col->addWidget(monthLab);

            for (int idx : monthRows.value(month)) {
                auto* item = new QWidget(colWrap);
                item->setLayoutDirection(Qt::LeftToRight);
                auto* row = new QHBoxLayout(item);
                row->setContentsMargins(0, 0, 0, 0);
                row->setSpacing(6);
                auto* swatch = new QFrame(item);
                swatch->setFixedSize(10, 10);
                swatch->setAutoFillBackground(true);
                swatch->setStyleSheet(QString("background-color:%1; border:1px solid #d4dbea; border-radius:2px;").arg(colors[idx].name()));
                QString text = labels[idx].mid(labels[idx].indexOf(monthSeparator) + monthSeparator.size()).trimmed();
                auto* lab = new QLabel(text, item);
                lab->setWordWrap(true);
                lab->setAlignment(Qt::AlignCenter);
                lab->setStyleSheet("background:transparent;color:#ffffff;font-size:11px;");
                row->addStretch();
                row->addWidget(swatch, 0, Qt::AlignTop);
                row->addWidget(lab, 0, Qt::AlignTop);
                row->addStretch();
                col->addWidget(item);
            }
            col->addStretch();
            hl->addWidget(colWrap, 1);
        }
        return wrap;
    }

    auto* wrap = new QWidget(parent);
    wrap->setLayoutDirection(Qt::LeftToRight);
    auto* grid = new QGridLayout(wrap);
    grid->setContentsMargins(8, 4, 8, 8);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(6);

    const int cols = labels.size() > 4 ? 2 : 3;
    for (int i = 0; i < labels.size() && i < colors.size(); ++i) {
        auto* item = new QWidget(wrap);
        item->setLayoutDirection(Qt::LeftToRight);
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
        addPieChart(tr_sales_4af850(), months, monthVals(&MonthData::sales), M_SALES);
    if (d.sel[M_SALES].candle)
        addCandleChart(tr_sales_4af850(), months, monthVals(&MonthData::sales), M_SALES);
    if (d.sel[M_SALES].line)
        addLineChart(tr_sales_4af850(), months, monthVals(&MonthData::sales), M_SALES);

    // ── Sales Return ──────────────────────────────────────────────────────
    if (d.sel[M_SALES_RETURN].pie)
        addPieChart(tr_sales_return_7b335a(),
                    months, monthVals(&MonthData::salesReturn), M_SALES_RETURN);
    if (d.sel[M_SALES_RETURN].candle)
        addCandleChart(tr_sales_return_7b335a(),
                       months, monthVals(&MonthData::salesReturn), M_SALES_RETURN);
    if (d.sel[M_SALES_RETURN].line)
        addLineChart(tr_sales_return_e520e9(),
                     months, monthVals(&MonthData::salesReturn), M_SALES_RETURN);

    // ── Purchases ─────────────────────────────────────────────────────────
    if (d.sel[M_PURCHASES].pie) {
        QList<double> combined;
        for (auto& m : d.months) combined << (m.supplierPurchases + m.supplierPayments);
        addPieChart(tr_purchases_988898(), months, combined, M_PURCHASES);
    }
    if (d.sel[M_PURCHASES].candle)
        addCandleChart(tr_purchases_988898(),
                       months, monthVals(&MonthData::supplierPurchases), M_PURCHASES);
    if (d.sel[M_PURCHASES].line)
        addLineChart(tr_purchases_16236c(),
                     months, monthVals(&MonthData::supplierPurchases), M_PURCHASES);

    // ── Expenses ──────────────────────────────────────────────────────────
    if (d.sel[M_EXPENSES].pie)
        addPieChart(tr_expenses_ed49c9(),
                    months, monthVals(&MonthData::expenseAmount), M_EXPENSE_AMOUNT);
    if (d.sel[M_EXPENSES].candle)
        addCandleChart(tr_expenses_ed49c9(),
                       months, monthVals(&MonthData::expenseAmount), M_EXPENSE_AMOUNT);
    if (d.sel[M_EXPENSES].line)
        addLineChart(tr_expenses_5a0c3c(),
                     months, monthVals(&MonthData::expenseAmount), M_EXPENSE_AMOUNT);

    // ── Inventory ─────────────────────────────────────────────────────────
    if (d.sel[M_INVENTORY].pie) {
        QList<double> combined;
        for (auto& m : d.months) combined << (m.inventoryFirst + m.inventoryLast);
        addPieChart(tr_inventory_d636e9(), months, combined, M_INVENTORY);
    }
    if (d.sel[M_INVENTORY].candle)
        addCandleChart(tr_inventory_d636e9(),
                       months, monthVals(&MonthData::inventoryFirst), M_INVENTORY_OPENING);
    if (d.sel[M_INVENTORY].line)
        addLineChart(tr_inventory_18734c(),
                     months, monthVals(&MonthData::inventoryFirst), M_INVENTORY_OPENING);

    // ── Net Sales ─────────────────────────────────────────────────────────
    if (d.sel[M_NET_SALES].pie)
        addPieChart(tr_net_sales_ae3003(),
                    months, arr12(d.netSales), M_NET_SALES);
    if (d.sel[M_NET_SALES].candle)
        addCandleChart(tr_net_sales_ae3003(),
                       months, arr12(d.netSales), M_NET_SALES);
    if (d.sel[M_NET_SALES].line)
        addLineChart(tr_net_sales_23a2f1(),
                     months, arr12(d.netSales), M_NET_SALES);

    // ── COGS ──────────────────────────────────────────────────────────────
    if (d.sel[M_COGS].pie)
        addPieChart(tr_cost_of_goods_sold_31b73d(),
                    months, arr12(d.cogs), M_COGS);
    if (d.sel[M_COGS].candle)
        addCandleChart(tr_cost_of_goods_sold_31b73d(),
                       months, arr12(d.cogs), M_COGS);
    if (d.sel[M_COGS].line)
        addLineChart(tr_cost_of_goods_sold_6e7684(),
                     months, arr12(d.cogs), M_COGS);

    // ── Profit Margin ─────────────────────────────────────────────────────
    if (d.sel[M_PROFIT_MARGIN].pie)
        addPieChart(tr_profit_margin_ff57d3(),
                    months, arr12(d.profitMargin), M_PROFIT_MARGIN);
    if (d.sel[M_PROFIT_MARGIN].candle)
        addCandleChart(tr_profit_margin_ff57d3(),
                       months, arr12(d.profitMargin), M_PROFIT_MARGIN);
    if (d.sel[M_PROFIT_MARGIN].line)
        addLineChart(tr_profit_margin_ec3b22(),
                     months, arr12(d.profitMargin), M_PROFIT_MARGIN);
}

// ─────────────────────────────────────────────────────────────────────────────
void ChartsWidget::addPieChart(const QString& title,
                                const QStringList& labels,
                                const QList<double>& values,
                                MetricId metricId)
{
    auto* cv = makePie(title, labels, values, metricId);
    m_views << cv;
    QList<QColor> colors;
    for (int i = 0; i < labels.size() && i < values.size(); ++i)
        colors << paletteColor(i);
    auto* wrap = wrapChartWithLegend(m_container, cv, labels, colors);
    m_layout->insertWidget(m_layout->count()-1, wrap);
}

void ChartsWidget::addCandleChart(const QString& title,
                                   const QStringList& labels,
                                   const QList<double>& values,
                                   MetricId metricId)
{
    auto* cv = makeCandle(title, labels, values, metricId);
    m_views << cv;
    auto* wrap = wrapChartWithLegend(m_container, cv,
                                     QStringList{tr_increasing_faa4d2(), tr_decreasing_d64136()},
                                     QList<QColor>{metricColor(metricId), contrastDecreaseColor(metricColor(metricId))});
    m_layout->insertWidget(m_layout->count()-1, wrap);
}

void ChartsWidget::addLineChart(const QString& title,
                                const QStringList& labels,
                                const QList<double>& values,
                                MetricId metricId)
{
    auto* cv = makeLine(title, labels, values, metricId);
    m_views << cv;
    auto* wrap = wrapChartWithLegend(m_container, cv,
                                     QStringList{title},
                                     QList<QColor>{metricColor(metricId)});
    m_layout->insertWidget(m_layout->count()-1, wrap);
}

// ─────────────────────────────────────────────────────────────────────────────
QChartView* ChartsWidget::makePie(const QString& title,
                                   const QStringList& labels,
                                   const QList<double>& values,
                                   MetricId metricId)
{
    auto* series = new QPieSeries;
    double total = 0;
    for (double v : values) total += qAbs(v);
    if (total == 0) total = 1;

    QList<QColor> sliceColors;
    for (int i = 0; i < labels.size() && i < values.size(); ++i) {
        double val = qAbs(values[i]);
        if (val == 0) continue;
        const QColor c = paletteColor(i);
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
                                      const QList<double>& values,
                                      MetricId metricId)
{
    auto* series = new QCandlestickSeries;
    series->setName(title);
    series->setIncreasingColor(metricColor(metricId));
    series->setDecreasingColor(contrastDecreaseColor(metricColor(metricId)));

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

    auto* axisY = new QCategoryAxis;
    axisY->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisY->setLabelsColor(Qt::white);
    axisY->setGridLineColor(QColor("#3a3f55"));
    double maxV = 0.0;
    for (double v : values) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    setCompactMoneyAxisRange(axisY, maxV);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Legend: add two invisible dummy series so the colour key is visible
    auto* legendInc = new QLineSeries;
    legendInc->setName(tr_increasing_c5cd67());
    legendInc->setColor(metricColor(metricId));
    auto* legendDec = new QLineSeries;
    legendDec->setName(tr_decreasing_b4c279());
    legendDec->setColor(contrastDecreaseColor(metricColor(metricId)));
    chart->addSeries(legendInc);
    chart->addSeries(legendDec);
    legendInc->attachAxis(axisX);
    legendInc->attachAxis(axisY);
    legendDec->attachAxis(axisX);
    legendDec->attachAxis(axisY);

    chart->legend()->setVisible(false);
    applyLegendMarkerColorsLater(chart, legendInc, QStringList{tr_increasing_c5cd67()}, QList<QColor>{metricColor(metricId)});
    applyLegendMarkerColorsLater(chart, legendDec, QStringList{tr_decreasing_b4c279()}, QList<QColor>{contrastDecreaseColor(metricColor(metricId))});
    chart->setMargins(QMargins(2, 2, 2, 20));
    chart->setAnimationOptions(QChart::AllAnimations);

    auto* view = makeSafeView(chart);
    view->setFixedSize(420, 360);
    view->setObjectName("chartView");
    QStringList candleLegendLabels;
    QStringList candleLegendColors;
    double candleTotal = 0.0;
    for (double v : values) candleTotal += qAbs(v);
    if (candleTotal < 0.001) candleTotal = 1.0;
    for (int i = 0; i < labels.size() && i < values.size(); ++i) {
        candleLegendLabels << (labels[i] + QStringLiteral(" — ") + title);
        candleLegendColors << ((values[i] >= 0.0) ? metricColor(metricId).name() : contrastDecreaseColor(metricColor(metricId)).name());
    }
    view->setProperty("legendLabels", candleLegendLabels);
    view->setProperty("legendColors", candleLegendColors);
    return view;
}


QChartView* ChartsWidget::makeLine(const QString& title,
                                   const QStringList& labels,
                                   const QList<double>& values,
                                   MetricId metricId)
{
    auto* line = new QLineSeries;
    line->setName(title);
    line->setColor(metricColor(metricId));
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

    auto* axisY = new QCategoryAxis;
    axisY->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    axisY->setLabelsColor(Qt::white);
    axisY->setGridLineColor(QColor("#3a3f55"));
    double maxV = 0.0;
    for (double v : values) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    setCompactMoneyAxisRange(axisY, maxV);
    chart->addAxis(axisY, Qt::AlignLeft);
    line->attachAxis(axisY);

    auto* view = makeSafeView(chart);
    view->setFixedSize(420, 360);
    view->setObjectName("chartView");
    QStringList lineLegendLabels;
    QStringList lineLegendColors;
    double lineTotal = 0.0;
    for (double v : values) lineTotal += qAbs(v);
    if (lineTotal < 0.001) lineTotal = 1.0;
    for (int i = 0; i < labels.size() && i < values.size(); ++i) {
        const double pct = (qAbs(values[i]) / lineTotal) * 100.0;
        lineLegendLabels << (labels[i] + QStringLiteral(" — ") + title);
        lineLegendColors << metricColor(metricId).name();
    }
    view->setProperty("legendLabels", lineLegendLabels);
    view->setProperty("legendColors", lineLegendColors);
    return view;
}
