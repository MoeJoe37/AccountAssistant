#include "pdfexporter.h"
#include "translations.h"
#include "resultswidget.h"

#include <QPdfWriter>
#include <QPainter>
#include <QChart>
#include <QPieSeries>
#include <QPieSlice>
#include <QLegendMarker>
#include <QChartView>
#include <QPageSize>
#include <QPageLayout>
#include <QDateTime>
#include <QImage>
#include <QColor>
#include <QFont>
#include <QRect>
#include <QRectF>
#include <QFontMetrics>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QVector>
#include <QFileInfo>
#include <QtMath>
#include <algorithm>
#include <cmath>

static const QColor kBg      ("#ffffff");
static const QColor kSurface ("#f4f6fb");
static const QColor kBorder  ("#dde2f0");
static const QColor kText    ("#1e2340");
static const QColor kMuted   ("#6b7280");
static const QColor kAccent  ("#4f86f7");
static const QColor kGreen   ("#2a9d5c");
static const QColor kAmber   ("#d97706");
static const QColor kRed     ("#e05c6a");
static const QColor kRowEven ("#f8f9ff");
static const QColor kRowOdd  ("#ffffff");
static const QColor kHdrBg   ("#eef0fa");

struct ChartMeta {
    QString title;
    QString type;
    QStringList labels;
    QList<double> values;
    QStringList labels2;
    QList<double> values2;
    QString nameA;
    QString nameB;
};

struct ValueStats {
    double total = 0.0;
    double avg = 0.0;
    double min = 0.0;
    double max = 0.0;
    int minIdx = -1;
    int maxIdx = -1;
};

static QString money(double v)
{
    return QString("$%L1").arg(v, 0, 'f', 0);
}

static ValueStats statsFor(const QList<double>& values)
{
    ValueStats s;
    if (values.isEmpty()) return s;
    s.min = s.max = values.first();
    s.minIdx = s.maxIdx = 0;
    for (int i = 0; i < values.size(); ++i) {
        const double v = values[i];
        s.total += v;
        if (v < s.min) { s.min = v; s.minIdx = i; }
        if (v > s.max) { s.max = v; s.maxIdx = i; }
    }
    s.avg = s.total / qMax(1, values.size());
    return s;
}

static QList<double> seriesForMetric(const AppData& d, MetricId id, QStringList* labels = nullptr, const QList<int>* months = nullptr, AccountTypeFilter accountFilter = AccountTypeFilter::All)
{
    return metricSeriesValues(d, id, labels, months, accountFilter);
}

static ChartMeta metaForRequest(const AppData& d, const ChartRequest& req)
{
    ChartMeta m;
    m.title = req.title.isEmpty() ? metricDisplayName(req.metricA) : req.title;
    m.nameA = req.seriesA.isEmpty() ? metricDisplayName(req.metricA) : req.seriesA;
    m.nameB = req.seriesB.isEmpty() ? metricDisplayName(req.metricB) : req.seriesB;
    const QList<int>* months = req.months.isEmpty() ? nullptr : &req.months;

    switch (req.kind) {
    case ChartKind::Pie:
        m.type = "pie";
        m.values = seriesForMetric(d, req.metricA, &m.labels, months, req.accountFilter);
        break;
    case ChartKind::Candle:
        if (!req.seriesB.isEmpty()) {
            m.type = "comparecandle";
            m.values = seriesForMetric(d, req.metricA, &m.labels, months, req.accountFilter);
            m.values2 = seriesForMetric(d, req.metricB, &m.labels2, months, req.accountFilter);
            if (m.labels2.isEmpty()) m.labels2 = m.labels;
        } else {
            m.type = (req.metricA == M_EXPENSES) ? "rankedbar" : "candle";
            m.values = seriesForMetric(d, req.metricA, &m.labels, months, req.accountFilter);
        }
        break;
    case ChartKind::RankedBar:
        m.type = "rankedbar";
        m.values = seriesForMetric(d, req.metricA, &m.labels, months, req.accountFilter);
        break;
    case ChartKind::CompareBar:
        m.type = "comparebar";
        m.values = seriesForMetric(d, req.metricA, &m.labels, months, req.accountFilter);
        m.values2 = seriesForMetric(d, req.metricB, &m.labels2, months, req.accountFilter);
        if (m.labels2.isEmpty()) m.labels2 = m.labels;
        break;
    case ChartKind::CompareLine:
        m.type = "compareline";
        m.values = seriesForMetric(d, req.metricA, &m.labels, months, req.accountFilter);
        m.values2 = seriesForMetric(d, req.metricB, &m.labels2, months, req.accountFilter);
        if (m.labels2.isEmpty()) m.labels2 = m.labels;
        break;
    case ChartKind::ComparePie: {
        m.type = "comparepie";
        const QList<double> a = seriesForMetric(d, req.metricA, &m.labels, months, req.accountFilter);
        const QList<double> b = seriesForMetric(d, req.metricB, &m.labels2, months, req.accountFilter);
        double ta = 0.0, tb = 0.0;
        for (double v : a) ta += qAbs(v);
        for (double v : b) tb += qAbs(v);
        m.labels = QStringList{m.nameA, m.nameB};
        m.values = QList<double>{ta, tb};
        m.values2.clear();
        break;
    }
    }
    return m;
}

static void drawMetricCard(QPainter& p, const QRect& rect, const QString& label,
                           const QString& value, const QColor& accent)
{
    p.fillRect(rect, kBg);
    p.setPen(QPen(kBorder, 1));
    p.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 10, 10);
    p.fillRect(QRect(rect.left(), rect.top(), 4, rect.height()), accent);
    p.setPen(kMuted);
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.drawText(QRect(rect.left() + 14, rect.top() + 10, rect.width() - 20, 18), Qt::AlignLeft | Qt::AlignVCenter, label);
    p.setPen(accent);
    p.setFont(QFont("Segoe UI", 16, QFont::Black));
    p.drawText(QRect(rect.left() + 14, rect.top() + 28, rect.width() - 20, rect.height() - 34), Qt::AlignLeft | Qt::AlignVCenter, value);
}

static void drawDataTable(QPainter& p, const QRect& rect, const ChartMeta& meta)
{
    const bool compare = !meta.values2.isEmpty();
    const int titleH = 24;
    const int hdrH   = 26;
    const int rowH   = 21;
    p.fillRect(rect, kBg);
    p.setPen(QPen(kBorder, 1));
    p.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 8, 8);
    QRect inner = rect.adjusted(8, 8, -8, -8);
    p.setPen(kAccent);
    p.setFont(QFont("Segoe UI", 10, QFont::Bold));
    p.drawText(QRect(inner.left(), inner.top(), inner.width(), titleH), Qt::AlignLeft | Qt::AlignVCenter,
               tr_data_breakdown_b66bb7());
    QRect tableRect = QRect(inner.left(), inner.top() + titleH + 4, inner.width(), inner.height() - titleH - 4);
    if (tableRect.height() <= hdrH) return;
    p.fillRect(QRect(tableRect.left(), tableRect.top(), tableRect.width(), hdrH), kHdrBg);
    p.setPen(QPen(kBorder, 1));
    p.drawRect(QRect(tableRect.left(), tableRect.top(), tableRect.width(), hdrH));
    p.setFont(QFont("Segoe UI", 8, QFont::Bold));
    p.setPen(kAccent);
    if (compare) {
        const int c1 = tableRect.width() * 34 / 100;
        const int c2 = tableRect.width() * 31 / 100;
        p.drawText(QRect(tableRect.left() + 4, tableRect.top(), c1 - 8, hdrH), Qt::AlignVCenter | Qt::AlignLeft, tr_month_9a21b7());
        p.drawText(QRect(tableRect.left() + c1 + 2, tableRect.top(), c2 - 6, hdrH), Qt::AlignVCenter | Qt::AlignRight, meta.nameA);
        p.drawText(QRect(tableRect.left() + c1 + c2 + 2, tableRect.top(), tableRect.width() - c1 - c2 - 4, hdrH), Qt::AlignVCenter | Qt::AlignRight, meta.nameB);
    } else {
        const int c1 = tableRect.width() * 55 / 100;
        p.drawText(QRect(tableRect.left() + 4, tableRect.top(), c1 - 8, hdrH), Qt::AlignVCenter | Qt::AlignLeft, tr_label_cd5fe4());
        p.drawText(QRect(tableRect.left() + c1, tableRect.top(), tableRect.width() - c1 - 4, hdrH), Qt::AlignVCenter | Qt::AlignRight, tr_value_dbccfd());
    }

    const int maxRows = qMax(0, (tableRect.height() - hdrH) / rowH);
    const int n = qMin(meta.labels.size(), maxRows);
    int y = tableRect.top() + hdrH;
    p.setFont(QFont("Segoe UI", 8));
    for (int i = 0; i < n; ++i) {
        p.fillRect(QRect(tableRect.left(), y, tableRect.width(), rowH), (i % 2 == 0) ? kRowEven : kRowOdd);
        p.setPen(QPen(kBorder, 1));
        p.drawLine(tableRect.left(), y + rowH, tableRect.right(), y + rowH);
        p.setPen(kText);
        if (compare) {
            const int c1 = tableRect.width() * 34 / 100;
            const int c2 = tableRect.width() * 31 / 100;
            p.drawText(QRect(tableRect.left() + 4, y, c1 - 8, rowH), Qt::AlignVCenter | Qt::AlignLeft, meta.labels.value(i));
            p.drawText(QRect(tableRect.left() + c1 + 2, y, c2 - 6, rowH), Qt::AlignVCenter | Qt::AlignRight, money(meta.values.value(i)));
            p.drawText(QRect(tableRect.left() + c1 + c2 + 2, y, tableRect.width() - c1 - c2 - 4, rowH), Qt::AlignVCenter | Qt::AlignRight, money(meta.values2.value(i)));
        } else {
            const int c1 = tableRect.width() * 55 / 100;
            const QString lbl = QFontMetrics(QFont("Segoe UI", 8)).elidedText(meta.labels.value(i), Qt::ElideRight, c1 - 12);
            p.drawText(QRect(tableRect.left() + 4, y, c1 - 8, rowH), Qt::AlignVCenter | Qt::AlignLeft, lbl);
            p.drawText(QRect(tableRect.left() + c1, y, tableRect.width() - c1 - 4, rowH), Qt::AlignVCenter | Qt::AlignRight, money(meta.values.value(i)));
        }
        y += rowH;
    }
}

static QImage renderPieChartPreview(const ChartMeta& meta, const QSize& size)
{
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    auto* series = new QPieSeries;
    double total = 0.0;
    for (double v : meta.values) total += qAbs(v);
    if (total < 0.001) total = 1.0;

    QVector<QColor> colors;
    for (int i = 0; i < meta.labels.size() && i < meta.values.size(); ++i) {
        const double v = qAbs(meta.values[i]);
        if (v < 0.001) continue;
        const QColor col = QColor::fromHsv((i * 40) % 360, 170, 220);
        colors << col;
        auto* sl = series->append(meta.labels[i], v);
        sl->setColor(col);
        sl->setBorderColor(kBg);
        sl->setLabelVisible(true);
        sl->setLabel(QString("%1%").arg(v / total * 100.0, 0, 'f', 1));
        sl->setLabelPosition(QPieSlice::LabelOutside);
        sl->setLabelArmLengthFactor(0.22);
        sl->setLabelColor(kText);
    }
    series->setPieSize(0.74);

    auto* chart = new QChart;
    chart->addSeries(series);
    chart->legend()->setVisible(false);
    chart->setBackgroundBrush(QBrush(kBg));
    chart->setPlotAreaBackgroundVisible(false);
    chart->setMargins(QMargins(6, 6, 6, 38));

    QChartView view(chart);
    view.setRenderHint(QPainter::Antialiasing, true);
    view.setRenderHint(QPainter::TextAntialiasing, true);
    view.resize(size);

    QPainter painter(&img);
    view.render(&painter, QRect(QPoint(0, 0), size));

    // Manual legend row so names stay visible and do not get replaced by percentages.
    const int legendY = size.height() - 28;
    const int swatch = 10;
    const int gap = 10;
    int x = 10;
    painter.setPen(kText);
    painter.setFont(QFont("Segoe UI", 8));
    for (int i = 0; i < meta.labels.size() && i < colors.size(); ++i) {
        const QString label = meta.labels[i];
        const int itemW = 12 + painter.fontMetrics().horizontalAdvance(label) + 18;
        if (x + itemW > size.width() - 10)
            break;
        painter.fillRect(QRect(x, legendY + 2, swatch, swatch), colors[i]);
        painter.drawRect(QRect(x, legendY + 2, swatch, swatch));
        painter.drawText(QRect(x + 14, legendY, itemW - 14, 16), Qt::AlignVCenter | Qt::AlignLeft, label);
        x += itemW + gap;
    }
    return img;
}

static void drawLegendRow(QPainter& p, const QRect& rect, const QList<QPair<QString, QColor>>& items)
{
    if (items.isEmpty()) return;
    const int swatch = 10;
    const int gap = 12;
    int x = rect.left();
    p.setFont(QFont("Segoe UI", 8));
    for (const auto& item : items) {
        const QString label = item.first;
        const int itemW = 12 + p.fontMetrics().horizontalAdvance(label) + 18;
        if (x + itemW > rect.right())
            break;
        p.fillRect(QRect(x, rect.top() + 2, swatch, swatch), item.second);
        p.setPen(QPen(kBorder, 1));
        p.drawRect(QRect(x, rect.top() + 2, swatch, swatch));
        p.setPen(kText);
        p.drawText(QRect(x + 14, rect.top(), itemW - 14, 16), Qt::AlignVCenter | Qt::AlignLeft, label);
        x += itemW + gap;
    }
}

static void drawPieLabel(QPainter& p, const QRectF& pie, double startDeg, double spanDeg, const QString& label, const QColor& color)
{
    constexpr double kPi = 3.14159265358979323846;
    const double mid = (startDeg + spanDeg / 2.0) * kPi / (180.0 * 16.0);
    const double radius = qMin(pie.width(), pie.height()) * 0.28;
    QPointF pos(pie.center().x() + std::cos(mid) * radius,
                pie.center().y() - std::sin(mid) * radius);
    p.setPen(color);
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    const QRectF textRect(pos.x() - 28, pos.y() - 10, 56, 20);
    p.drawText(textRect, Qt::AlignCenter, label);
}

static void drawPiePreview(QPainter& p, const QRect& rect, const ChartMeta& meta)
{
    const QRect chartRect = rect.adjusted(10, 10, -10, -10);
    const QImage img = renderPieChartPreview(meta, chartRect.size());
    p.drawImage(chartRect.topLeft(), img);
}

static void drawComparePiePreview(QPainter& p, const QRect& rect, const ChartMeta& meta)
{
    ChartMeta m = meta;
    m.type = "pie";
    drawPiePreview(p, rect, m);
}

static void drawBarPreview(QPainter& p, const QRect& rect, const ChartMeta& meta, bool grouped)
{
    if (meta.values.isEmpty()) return;
    double maxV = 0.0; for (double v : meta.values) maxV = qMax(maxV, qAbs(v));
    if (grouped) for (double v : meta.values2) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    QRect chart = rect.adjusted(14, 14, -14, -14);
    const int leftPad = 42, bottomPad = 28, topPad = 16;
    QRect plot(chart.left() + leftPad, chart.top() + topPad, chart.width() - leftPad - 6, chart.height() - topPad - bottomPad);
    p.setPen(QPen(kBorder, 1));
    p.drawRect(plot);
    p.setPen(kMuted);
    p.setFont(QFont("Segoe UI", 8));
    p.drawText(QRect(chart.left(), chart.top(), chart.width(), 14), Qt::AlignLeft, tr_timeline_7ab9a0());
    const int n = meta.values.size();
    const double step = double(plot.width()) / qMax(1, n);
    const double barW = grouped ? qMax(4.0, step * 0.22) : qMax(8.0, step * 0.6);
    for (int i = 0; i < n; ++i) {
        const double h = plot.height() * (qAbs(meta.values[i]) / maxV);
        QRectF bar(plot.left() + i * step + (step - barW) / 2.0, plot.bottom() - h, barW, h);
        p.fillRect(bar, grouped ? QColor::fromHsv((i * 35) % 360, 170, 220) : kAccent);
    }
    if (grouped && !meta.values2.isEmpty()) {
        for (int i = 0; i < qMin(meta.values2.size(), n); ++i) {
            const double h = plot.height() * (qAbs(meta.values2[i]) / maxV);
            QRectF bar(plot.left() + i * step + (step - barW) / 2.0 + barW + 4, plot.bottom() - h, barW, h);
            p.fillRect(bar, kGreen);
        }
    }
    p.setPen(kMuted);
    p.setFont(QFont("Segoe UI", 8));
    const int maxLabels = qMin(n, 12);
    for (int i = 0; i < maxLabels; ++i) {
        const int x = plot.left() + int(i * step + step * 0.5) - 30;
        p.drawText(QRect(x, plot.bottom() + 4, 60, 18), Qt::AlignHCenter, meta.labels.value(i));
    }

    const QRect legendRect(chart.left() + 10, chart.bottom() - 18, chart.width() - 20, 16);
    drawLegendRow(p, legendRect, QList<QPair<QString, QColor>>{
        { tr_increasing_da37b2(), kGreen },
        { tr_decreasing_606062(), kRed },
    });
}

static void drawCompareCandles(QPainter& p, const QRect& rect, const ChartMeta& meta)
{
    if (meta.values.isEmpty() || meta.values2.isEmpty()) return;
    double maxV = 0.0;
    for (double v : meta.values) maxV = qMax(maxV, qAbs(v));
    for (double v : meta.values2) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    QRect chart = rect.adjusted(18, 18, -18, -18);
    const int leftPad = 48;
    const int bottomPad = 34;
    QRect plot(chart.left() + leftPad, chart.top() + 10, chart.width() - leftPad - 8, chart.height() - bottomPad - 14);
    p.setPen(QPen(kBorder, 1));
    p.drawRect(plot);
    p.setFont(QFont("Segoe UI", 8));
    p.setPen(kMuted);
    p.drawText(QRect(chart.left(), chart.top(), chart.width(), 14), Qt::AlignLeft, tr_timeline_22644f());
    const int n = qMax(meta.values.size(), meta.values2.size());
    const double step = double(plot.width()) / qMax(1, n);
    const double bodyW = qMax(4.0, step * 0.24);
    const double offset = qMin(8.0, step * 0.18);
    auto drawOne = [&](const QList<double>& vals, const QColor& col, double xOff) {
        for (int i = 0; i < vals.size(); ++i) {
            const double open = 0.0;
            const double close = vals[i];
            const double hi = qMax(open, close) * 1.02 + 0.01;
            const double lo = qMin(open, close) * 0.98;
            const double yHi = plot.bottom() - plot.height() * (qAbs(hi) / maxV);
            const double yLo = plot.bottom() - plot.height() * (qAbs(lo) / maxV);
            const double yOpen = plot.bottom() - plot.height() * (qAbs(open) / maxV);
            const double yClose = plot.bottom() - plot.height() * (qAbs(close) / maxV);
            const bool rising = close >= open;
            p.setPen(QPen(rising ? col : col.darker(120), 2));
            const double midX = plot.left() + i * step + step / 2.0 + xOff;
            p.drawLine(QPointF(midX, yHi), QPointF(midX, yLo));
            QRectF body(midX - bodyW / 2.0, qMin(yOpen, yClose), bodyW, qMax(3.0, qAbs(yClose - yOpen)));
            p.fillRect(body, rising ? col : col.darker(120));
        }
    };
    drawOne(meta.values, kAmber, -offset);
    drawOne(meta.values2, kGreen, offset);
    p.setPen(kMuted);
    p.setFont(QFont("Segoe UI", 8));
    const int maxLabels = qMin(n, 12);
    for (int i = 0; i < maxLabels; ++i) {
        const int x = plot.left() + int(i * step + step * 0.5) - 30;
        p.drawText(QRect(x, plot.bottom() + 4, 60, 18), Qt::AlignHCenter, meta.labels.value(i));
    }

    const QRect legendRect(chart.left() + 10, chart.bottom() - 18, chart.width() - 20, 16);
    drawLegendRow(p, legendRect, QList<QPair<QString, QColor>>{
        { meta.nameA.isEmpty() ? tr_series_a_2b8d21() : meta.nameA, kAmber },
        { meta.nameB.isEmpty() ? tr_series_b_b63de0() : meta.nameB, kGreen },
    });
}

static void drawRankedBars(QPainter& p, const QRect& rect, const ChartMeta& meta)
{
    if (meta.values.isEmpty()) return;
    double maxV = 0.0; for (double v : meta.values) maxV = qMax(maxV, qAbs(v)); if (maxV < 0.001) maxV = 1.0;
    QRect chart = rect.adjusted(16, 16, -16, -16);
    const int rowH = qMax(18, chart.height() / qMax(1, meta.values.size()));
    const int barsX = chart.left() + 130;
    const int barsW = chart.width() - 140;
    p.setFont(QFont("Segoe UI", 8));
    for (int i = 0; i < meta.values.size(); ++i) {
        const int y = chart.top() + i * rowH;
        p.fillRect(QRect(chart.left(), y, chart.width(), rowH), (i % 2 == 0) ? kRowEven : kRowOdd);
        p.setPen(kText);
        p.drawText(QRect(chart.left() + 4, y, 120, rowH), Qt::AlignVCenter | Qt::AlignLeft, QString::number(i + 1) + QStringLiteral(". ") + meta.labels.value(i));
        QRectF bar(barsX, y + 4, barsW * (qAbs(meta.values[i]) / maxV), rowH - 8);
        p.fillRect(bar, kAccent);
        p.setPen(kMuted);
        p.drawText(QRect(barsX + 6, y, barsW - 6, rowH), Qt::AlignVCenter | Qt::AlignLeft, money(meta.values[i]));
    }
}

static void drawCandles(QPainter& p, const QRect& rect, const ChartMeta& meta)
{
    if (meta.values.isEmpty()) return;
    double maxV = 0.0;
    for (double v : meta.values) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    QRect chart = rect.adjusted(18, 18, -18, -18);
    const int leftPad = 48;
    const int bottomPad = 34;
    QRect plot(chart.left() + leftPad, chart.top() + 10, chart.width() - leftPad - 8, chart.height() - bottomPad - 14);
    p.setPen(QPen(kBorder, 1));
    p.drawRect(plot);
    p.setFont(QFont("Segoe UI", 8));
    p.setPen(kMuted);
    p.drawText(QRect(chart.left(), chart.top(), chart.width(), 14), Qt::AlignLeft, tr_timeline_7ab9a0());
    const int n = meta.values.size();
    const double step = double(plot.width()) / qMax(1, n);
    const double bodyW = qMax(6.0, step * 0.45);
    for (int i = 0; i < n; ++i) {
        const double open = 0.0;
        const double close = meta.values[i];
        const double hi = qMax(open, close) * 1.02 + 0.01;
        const double lo = qMin(open, close) * 0.98;
        const double yHi = plot.bottom() - plot.height() * (qAbs(hi) / maxV);
        const double yLo = plot.bottom() - plot.height() * (qAbs(lo) / maxV);
        const double yOpen = plot.bottom() - plot.height() * (qAbs(open) / maxV);
        const double yClose = plot.bottom() - plot.height() * (qAbs(close) / maxV);
        const bool rising = close >= open;
        p.setPen(QPen(rising ? kGreen : kRed, 2));
        const double midX = plot.left() + i * step + step / 2.0;
        p.drawLine(QPointF(midX, yHi), QPointF(midX, yLo));
        QRectF body(midX - bodyW / 2.0, qMin(yOpen, yClose), bodyW, qMax(3.0, qAbs(yClose - yOpen)));
        p.fillRect(body, rising ? kGreen : kRed);
    }
    p.setPen(kMuted);
    p.setFont(QFont("Segoe UI", 8));
    const int maxLabels = qMin(n, 12);
    for (int i = 0; i < maxLabels; ++i) {
        const int x = plot.left() + int(i * step + step * 0.5) - 30;
        p.drawText(QRect(x, plot.bottom() + 4, 60, 18), Qt::AlignHCenter, meta.labels.value(i));
    }

    const QRect legendRect(chart.left() + 10, chart.bottom() - 18, chart.width() - 20, 16);
    drawLegendRow(p, legendRect, QList<QPair<QString, QColor>>{
        { tr_increasing_da37b2(), kGreen },
        { tr_decreasing_606062(), kRed },
    });
}

static void drawLineCompare(QPainter& p, const QRect& rect, const ChartMeta& meta)
{
    if (meta.values.isEmpty() || meta.values2.isEmpty()) return;
    double maxV = 0.0;
    for (double v : meta.values) maxV = qMax(maxV, qAbs(v));
    for (double v : meta.values2) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    QRect chart = rect.adjusted(16, 16, -16, -16);
    QRect plot(chart.left() + 48, chart.top() + 12, chart.width() - 58, chart.height() - 40);
    p.setPen(QPen(kBorder, 1));
    p.drawRect(plot);

    auto drawSeries = [&](const QList<double>& vals, const QColor& col) {
        QPolygonF poly;
        const int n = vals.size();
        const double step = double(plot.width()) / qMax(1, n - 1);
        for (int i = 0; i < n; ++i) {
            const double x = plot.left() + i * step;
            const double y = plot.bottom() - plot.height() * (qAbs(vals[i]) / maxV);
            poly << QPointF(x, y);
        }
        p.setPen(QPen(col, 2));
        p.drawPolyline(poly);
        p.setBrush(col);
        for (const auto& pt : poly)
            p.drawEllipse(pt, 3, 3);
    };

    drawSeries(meta.values, kAmber);
    drawSeries(meta.values2, kGreen);
    p.setPen(kMuted);
    p.setFont(QFont("Segoe UI", 8));
    const int n = qMin(meta.labels.size(), 12);
    const double step = double(plot.width()) / qMax(1, meta.values.size() - 1);
    for (int i = 0; i < n; ++i) {
        const int x = plot.left() + int(i * step) - 30;
        p.drawText(QRect(x, plot.bottom() + 4, 60, 18), Qt::AlignHCenter, meta.labels.value(i));
    }
}

static void drawChartPreview(QPainter& p, const QRect& chartArea, const ChartMeta& meta)
{
    p.fillRect(chartArea, Qt::white);
    if (meta.type == "pie") drawPiePreview(p, chartArea, meta);
    else if (meta.type == "comparepie") drawComparePiePreview(p, chartArea, meta);
    else if (meta.type == "rankedbar") drawRankedBars(p, chartArea, meta);
    else if (meta.type == "candle") drawCandles(p, chartArea, meta);
    else if (meta.type == "comparecandle") drawCompareCandles(p, chartArea, meta);
    else if (meta.type == "compareline") drawLineCompare(p, chartArea, meta);
    else drawBarPreview(p, chartArea, meta, true);
}

static QImage renderCoverPage(const AppData& data, const QSize& size)
{
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(kBg);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    p.fillRect(QRect(0, 0, size.width(), 80), kAccent);
    p.setFont(QFont("Segoe UI", 20, QFont::Black));
    p.setPen(Qt::white);
    p.drawText(QRect(48, 12, size.width() - 96, 56), Qt::AlignVCenter | Qt::AlignLeft,
               tr_account_assistant_financial_re_7851db());
    p.setFont(QFont("Segoe UI", 9));
    p.setPen(QColor(220, 230, 255));
    p.drawText(QRect(size.width() - 300, 20, 252, 40), Qt::AlignVCenter | Qt::AlignRight,
               QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm"));

    struct Card { const char* enLbl; const char* arLbl; double val; QColor col; };
    QList<Card> cards = {
        { "Net Sales", "\u0635\u0627\u0641\u064A \u0627\u0644\u0645\u0628\u064A\u0639\u0627\u062A", data.totalNetSales, kGreen },
        { "COGS", "\u062A\u0643\u0644\u0641\u0629 \u0627\u0644\u0628\u0636\u0627\u0639\u0629", data.totalCOGS, kAmber },
        { "Profit Margin", "\u0647\u0627\u0645\u0634 \u0627\u0644\u0631\u0628\u062D", data.totalProfit, data.totalProfit >= 0 ? kGreen : kRed },
    };
    const int mg = 48;
    const int y = 104;
    const int cw = (size.width() - 2 * mg - 24) / 3;
    for (int i = 0; i < cards.size(); ++i) {
        const int cx = mg + i * (cw + 12);
        p.fillRect(QRect(cx, y, cw, 90), kSurface);
        p.setPen(QPen(kBorder, 1));
        p.drawRect(QRect(cx, y, cw, 90));
        p.fillRect(QRect(cx, y, 4, 90), cards[i].col);
        p.setFont(QFont("Segoe UI", 8, QFont::Bold));
        p.setPen(kMuted);
        p.drawText(QRect(cx + 14, y + 12, cw - 20, 20), Qt::AlignLeft | Qt::AlignVCenter, T(cards[i].enLbl, cards[i].arLbl));
        p.setFont(QFont("Segoe UI", 17, QFont::Black));
        p.setPen(cards[i].col);
        p.drawText(QRect(cx + 14, y + 34, cw - 20, 46), Qt::AlignLeft | Qt::AlignVCenter, money(cards[i].val));
    }

    int sepY = y + 108;
    p.setPen(QPen(kBorder, 1));
    p.drawLine(mg, sepY, size.width() - mg, sepY);
    sepY += 18;

    if (!data.expenseSummary.isEmpty()) {
        sepY += 10;
        p.setFont(QFont("Segoe UI", 10, QFont::Bold));
        p.setPen(kAccent);
        p.drawText(QRect(mg, sepY, size.width() - 2 * mg, 22), Qt::AlignLeft,
                   tr_expense_accounts_ranked_34d3f9());
        sepY += 26;
        const int rows = qMin(data.expenseSummary.size(), 6);
        for (int i = 0; i < rows; ++i) {
            const auto& e = data.expenseSummary[i];
            QString line = QString::number(i + 1) + QStringLiteral(". ") + e.account + QStringLiteral("  —  ") + money(e.total);
            p.setPen(i % 2 == 0 ? kText : kMuted);
            p.drawText(QRect(mg + 10, sepY, size.width() - 2 * mg - 20, 20), Qt::AlignLeft, line);
            sepY += 22;
        }
    }
    return img;
}

static QImage renderMonthlyReportPage(const AppData& data, const QSize& size, const QList<int>& monthOrder)
{
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(kBg);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    p.fillRect(QRect(0, 0, size.width(), 74), kSurface);
    p.setPen(QPen(kBorder, 1));
    p.drawLine(0, 74, size.width(), 74);
    p.setFont(QFont("Segoe UI", 18, QFont::Black));
    p.setPen(kAccent);
    p.drawText(QRect(30, 10, size.width() - 60, 28), Qt::AlignLeft | Qt::AlignVCenter,
               tr_monthly_report_cards_0c6d88());
    p.setFont(QFont("Segoe UI", 9));
    p.setPen(kMuted);
    p.drawText(QRect(30, 36, size.width() - 60, 18), Qt::AlignLeft | Qt::AlignVCenter,
               tr_each_month_is_shown_as_an_indi_81dc38());
    p.drawText(QRect(size.width() - 270, 18, 240, 18), Qt::AlignRight | Qt::AlignVCenter,
               QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm"));

    QRect area(28, 100, size.width() - 56, size.height() - 128);
    const int cols = 3;
    const int rows = 4;
    const int gapX = 14;
    const int gapY = 14;
    const int cardW = (area.width() - gapX * (cols - 1)) / cols;
    const int cardH = (area.height() - gapY * (rows - 1)) / rows;

    auto drawCard = [&](const QRect& rect, const QString& month, double netSales, double cogs, double profit) {
        p.fillRect(rect, kSurface);
        p.setPen(QPen(kBorder, 1));
        p.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 12, 12);
        p.fillRect(QRect(rect.left(), rect.top(), 5, rect.height()), kAccent);

        QRect inner = rect.adjusted(14, 12, -14, -12);
        p.setPen(kText);
        p.setFont(QFont("Segoe UI", 12, QFont::Black));
        p.drawText(QRect(inner.left(), inner.top(), inner.width(), 20), Qt::AlignLeft | Qt::AlignVCenter, month);

        auto drawLine = [&](int y, const QString& label, const QString& value, const QColor& col) {
            p.setFont(QFont("Segoe UI", 8, QFont::Bold));
            p.setPen(kMuted);
            p.drawText(QRect(inner.left(), y, inner.width() - 72, 16), Qt::AlignLeft | Qt::AlignVCenter, label);
            p.setFont(QFont("Segoe UI", 11, QFont::Black));
            p.setPen(col);
            p.drawText(QRect(inner.left(), y, inner.width(), 16), Qt::AlignRight | Qt::AlignVCenter, value);
        };

        drawLine(inner.top() + 30, tr_net_sales_23a2f1(), money(netSales), netSales >= 0 ? kGreen : kRed);
        drawLine(inner.top() + 52, tr_cogs_d716f1(), money(cogs), kAmber);
        drawLine(inner.top() + 74, tr_profit_margin_ec3b22(), money(profit), profit >= 0 ? kGreen : kRed);
    };

    const auto months = monthNames();
    QList<int> order = monthOrder;
    if (order.size() != 12) {
        order.clear();
        for (int i = 0; i < 12; ++i) order << i;
    }
    for (int i = 0; i < order.size(); ++i) {
        const int monthIdx = order[i];
        const int r = i / cols;
        const int c = i % cols;
        QRect card(area.left() + c * (cardW + gapX), area.top() + r * (cardH + gapY), cardW, cardH);
        drawCard(card, months.value(monthIdx), data.netSales[monthIdx], data.cogs[monthIdx], data.profitMargin[monthIdx]);
    }
    return img;
}

static QImage renderChartPage(const AppData& data, const ChartRequest& req, const QSize& size)
{
    const ChartMeta meta = metaForRequest(data, req);
    QImage img(size, QImage::Format_ARGB32_Premultiplied);
    img.fill(kBg);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const int margin = 24;
    const int headerH = 58;
    const int metricsH = 84;
    const int gap = 14;
    const int bodyTop = margin + headerH + gap + metricsH + gap;
    const int bodyH = size.height() - bodyTop - margin;
    const int splitX = size.width() * 60 / 100;

    p.fillRect(QRect(0, 0, size.width(), margin + headerH), kSurface);
    p.setPen(QPen(kBorder, 1));
    p.drawLine(0, margin + headerH, size.width(), margin + headerH);
    p.setPen(kAccent);
    p.setFont(QFont("Segoe UI", 18, QFont::Black));
    p.drawText(QRect(margin, 14, size.width() - 2 * margin, 26), Qt::AlignLeft | Qt::AlignVCenter, meta.title);
    p.setPen(kMuted);
    p.setFont(QFont("Segoe UI", 9));
    p.drawText(QRect(margin, 36, size.width() - 2 * margin, 18), Qt::AlignLeft | Qt::AlignVCenter,
               tr_exported_as_a_detailed_static__365ac4());
    p.drawText(QRect(size.width() - 260, 18, 236, 22), Qt::AlignRight | Qt::AlignVCenter,
               QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm"));

    QRect m1(margin, margin + headerH + gap, (size.width() - 2 * margin - 3 * gap) / 4, metricsH);
    QRect m2(m1.right() + gap + 1, m1.top(), m1.width(), m1.height());
    QRect m3(m2.right() + gap + 1, m1.top(), m1.width(), m1.height());
    QRect m4(m3.right() + gap + 1, m1.top(), size.width() - margin - (m3.right() + gap + 1), m1.height());

    if (!meta.values2.isEmpty()) {
        const double totalA = statsFor(meta.values).total;
        const double totalB = statsFor(meta.values2).total;
        double avgGap = 0.0, maxGap = 0.0;
        const int n = qMin(meta.values.size(), meta.values2.size());
        for (int i = 0; i < n; ++i) {
            const double dV = qAbs(meta.values[i] - meta.values2[i]);
            avgGap += dV;
            maxGap = qMax(maxGap, dV);
        }
        if (n > 0) avgGap /= n;
        drawMetricCard(p, m1, meta.nameA, money(totalA), kAccent);
        drawMetricCard(p, m2, meta.nameB, money(totalB), kGreen);
        drawMetricCard(p, m3, tr_avg_gap_7363da(), money(avgGap), kAmber);
        drawMetricCard(p, m4, tr_max_gap_fd508b(), money(maxGap), kRed);
    } else {
        const ValueStats s = statsFor(meta.values);
        drawMetricCard(p, m1, tr_total_a52764(), money(s.total), kAccent);
        drawMetricCard(p, m2, tr_average_7302d6(), money(s.avg), kGreen);
        drawMetricCard(p, m3, tr_high_5ed23d(), money(s.max), kAmber);
        drawMetricCard(p, m4, tr_low_abc4e2(), money(s.min), kRed);
    }

    QRect chartPanel(margin, bodyTop, splitX - margin - gap / 2, bodyH);
    QRect tablePanel(splitX + gap / 2, bodyTop, size.width() - splitX - margin - gap / 2, bodyH);
    p.fillRect(chartPanel, kBg);
    p.setPen(QPen(kBorder, 1));
    p.drawRoundedRect(chartPanel.adjusted(0, 0, -1, -1), 8, 8);

    QRect chartInner = chartPanel.adjusted(12, 12, -12, -12);
    p.setPen(kAccent);
    p.setFont(QFont("Segoe UI", 10, QFont::Bold));
    p.drawText(QRect(chartInner.left(), chartInner.top(), chartInner.width(), 22), Qt::AlignLeft | Qt::AlignVCenter,
               tr_chart_preview_9abc22());
    QRect chartArea = chartInner.adjusted(0, 26, 0, 0);
    drawChartPreview(p, chartArea, meta);

    drawDataTable(p, tablePanel, meta);
    return img;
}

static void drawMonthFlowBlock(QPainter& p, const QRect& rect, const AppData& data, int monthIdx)
{
    const auto months = monthNames();
    const bool summary = (monthIdx == 0);
    p.fillRect(rect, kBg);
    p.setPen(QPen(kBorder, 1));
    p.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 12, 12);
    p.fillRect(QRect(rect.left(), rect.top(), 5, rect.height()), kAccent);

    QRect inner = rect.adjusted(14, 12, -14, -12);
    p.setPen(kText);
    p.setFont(QFont("Segoe UI", 12, QFont::Black));
    p.drawText(QRect(inner.left(), inner.top(), inner.width(), 20), Qt::AlignLeft | Qt::AlignVCenter,
               summary ? tr_all_months_428b74() : months.value(monthIdx - 1));

    auto drawLine = [&](int y, const QString& label, const QString& value, const QColor& col) {
        p.setFont(QFont("Segoe UI", 8, QFont::Bold));
        p.setPen(kMuted);
        p.drawText(QRect(inner.left(), y, inner.width() - 72, 16), Qt::AlignLeft | Qt::AlignVCenter, label);
        p.setFont(QFont("Segoe UI", 11, QFont::Black));
        p.setPen(col);
        p.drawText(QRect(inner.left(), y, inner.width(), 16), Qt::AlignRight | Qt::AlignVCenter, value);
    };

    if (summary) {
        drawLine(inner.top() + 30, tr_net_sales_23a2f1(), money(data.totalNetSales), data.totalNetSales >= 0 ? kGreen : kRed);
        drawLine(inner.top() + 52, tr_cogs_d716f1(), money(data.totalCOGS), kAmber);
        drawLine(inner.top() + 74, tr_profit_margin_ec3b22(), money(data.totalProfit), data.totalProfit >= 0 ? kGreen : kRed);
    } else {
        const int idx = monthIdx - 1;
        drawLine(inner.top() + 30, tr_net_sales_23a2f1(), money(data.netSales[idx]), data.netSales[idx] >= 0 ? kGreen : kRed);
        drawLine(inner.top() + 52, tr_cogs_d716f1(), money(data.cogs[idx]), kAmber);
        drawLine(inner.top() + 74, tr_profit_margin_ec3b22(), money(data.profitMargin[idx]), data.profitMargin[idx] >= 0 ? kGreen : kRed);
    }
}

static void drawChartFlowBlock(QPainter& p, const QRect& rect, const AppData& data, const ChartRequest& req)
{
    const ChartMeta meta = metaForRequest(data, req);
    p.fillRect(rect, kBg);
    p.setPen(QPen(kBorder, 1));
    p.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 12, 12);
    QRect header = rect.adjusted(14, 12, -14, -14);
    p.setPen(kAccent);
    p.setFont(QFont("Segoe UI", 13, QFont::Bold));
    p.drawText(QRect(header.left(), header.top(), header.width(), 22), Qt::AlignLeft | Qt::AlignVCenter, meta.title);
    p.setPen(kMuted);
    p.setFont(QFont("Segoe UI", 8));
    p.drawText(QRect(header.left(), header.top() + 22, header.width(), 16), Qt::AlignLeft | Qt::AlignVCenter,
               tr_chart_block_583f01());
    QRect chartArea = rect.adjusted(14, 42, -14, -14);
    drawChartPreview(p, chartArea, meta);
}


bool PdfExporter::exportToPdf(const QString& path, const AppData& data, const ResultsWidget* results, bool landscape)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
    writer.setResolution(144);
    writer.setCreator(QStringLiteral("Account Assistant"));
    writer.setTitle(QStringLiteral("Account Assistant Report"));

    QPainter p;
    if (!p.begin(&writer)) return false;

    const QSize pageSize(writer.width(), writer.height());
    if (pageSize.isEmpty()) {
        p.end();
        return false;
    }

    auto paintPage = [&](const QImage& img) {
        p.drawImage(QRect(0, 0, pageSize.width(), pageSize.height()), img);
    };

    paintPage(renderCoverPage(data, pageSize));
    writer.newPage();

    QList<ResultFlowItem> flow = results ? results->flowOrder() : data.resultFlowOrder;
    if (flow.isEmpty()) {
        flow.append(ResultFlowItem{ResultFlowItemKind::MonthCard, 0, -1});
        for (int i = 1; i <= 12; ++i) flow.append(ResultFlowItem{ResultFlowItemKind::MonthCard, i, -1});
        for (int i = 0; i < data.chartRequests.size(); ++i) flow.append(ResultFlowItem{ResultFlowItemKind::ChartCard, i, -1});
    }

    QImage page(pageSize, QImage::Format_ARGB32_Premultiplied);
    auto startPage = [&]() {
        page.fill(kBg);
        QPainter qp(&page);
        qp.setRenderHint(QPainter::Antialiasing, true);
        qp.setRenderHint(QPainter::TextAntialiasing, true);
        qp.fillRect(QRect(0, 0, pageSize.width(), 74), kSurface);
        qp.setPen(QPen(kBorder, 1));
        qp.drawLine(0, 74, pageSize.width(), 74);
        qp.setPen(kAccent);
        qp.setFont(QFont("Segoe UI", 18, QFont::Black));
        qp.drawText(QRect(30, 10, pageSize.width() - 60, 28), Qt::AlignLeft | Qt::AlignVCenter,
                    tr_results_flow_77d465());
        qp.setFont(QFont("Segoe UI", 9));
        qp.setPen(kMuted);
        qp.drawText(QRect(30, 36, pageSize.width() - 60, 18), Qt::AlignLeft | Qt::AlignVCenter,
                    QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm"));
    };

    startPage();
    int y = 96;
    const int margin = 28;
    const int contentW = pageSize.width() - 2 * margin;
    const int gap = 14;

    auto flushPage = [&]() {
        paintPage(page);
        writer.newPage();
        startPage();
        y = 96;
    };

    for (const auto& item : flow) {
        if (item.kind == ResultFlowItemKind::PageSeparator) {
            flushPage();
            continue;
        }

        if (!results) {
            // Fallback to the old drawn report if we do not have access to the live widgets.
            const int blockH = (item.kind == ResultFlowItemKind::MonthCard) ? 168 : 400;
            if (y + blockH > pageSize.height() - margin)
                flushPage();
            QPainter qp(&page);
            QRect block(margin, y, contentW, blockH);
            if (item.kind == ResultFlowItemKind::MonthCard && item.index >= 0 && item.index <= 12) {
                drawMonthFlowBlock(qp, block, data, item.index);
            } else if (item.kind == ResultFlowItemKind::ChartCard && item.index >= 0 && item.index < data.chartRequests.size()) {
                drawChartFlowBlock(qp, block, data, data.chartRequests[item.index]);
            }
            y += blockH + gap;
            continue;
        }

        QImage cardImg = results->renderFlowItemImage(item);
        if (cardImg.isNull())
            continue;

        const int maxBlockH = pageSize.height() - margin - y;
        QSize target(contentW, cardImg.height() * contentW / qMax(1, cardImg.width()));
        if (target.height() > maxBlockH && maxBlockH > 0) {
            target = cardImg.size().scaled(QSize(contentW, maxBlockH), Qt::KeepAspectRatio);
        }
        if (target.isEmpty())
            target = cardImg.size();

        if (y + target.height() > pageSize.height() - margin)
            flushPage();

        QPainter qp(&page);
        QRect block(margin, y, contentW, target.height());
        QImage scaled = cardImg.scaled(block.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPoint topLeft(block.left() + (block.width() - scaled.width()) / 2,
                             block.top() + (block.height() - scaled.height()) / 2);
        qp.drawImage(topLeft, scaled);
        y += scaled.height() + gap;
    }

    paintPage(page);

    p.end();
    return QFileInfo::exists(path) && QFileInfo(path).size() > 0;
}

