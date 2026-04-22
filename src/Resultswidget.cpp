#include "resultswidget.h"
#include "translations.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QCategoryAxis>
#include <QLineSeries>
#include <QFont>
#include <QColor>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDragMoveEvent>
#include <QLabel>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QContextMenuEvent>
#include <QTimer>
#include <QGridLayout>
#include <QScrollBar>
#include <QFrame>
#include <QToolButton>
#include <QAction>
#include <QActionGroup>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QSizePolicy>
#include <QSet>
#include <QTableWidgetItem>
#include <QPen>
#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

// ─── Stay-open menu (doesn't close when clicking checkable items) ─────────────
class StayOpenMenu : public QMenu {
public:
    using QMenu::QMenu;
protected:
    void mouseReleaseEvent(QMouseEvent* e) override {
        QAction* a = activeAction();
        if (a && a->isEnabled()) {
            if (a->isCheckable()) {
                a->trigger();
            } else {
                a->trigger();
                QMenu::mouseReleaseEvent(e); // non-checkable items close menu normally
                return;
            }
            return; // stay open for checkable items
        }
        QMenu::mouseReleaseEvent(e);
    }
};

static const QList<QColor> kPal = {
    "#4f86f7", "#f0a500", "#e05c6a", "#3ecf8e",
    "#9b6cf9", "#f06c6c", "#62c4e3", "#b0e96a",
    "#ff9f43", "#fd79a8", "#00cec9", "#fdcb6e"
};

static QList<double> normalizePercentValues(const QList<double>& raw)
{
    QList<double> out;
    if (raw.isEmpty()) return out;

    QVector<double> weights;
    weights.reserve(raw.size());
    double sum = 0.0;
    for (double v : raw) {
        const double w = qAbs(v);
        weights << w;
        sum += w;
    }

    if (sum < 0.0001) {
        double running = 0.0;
        const double base = 100.0 / raw.size();
        for (int i = 0; i < raw.size(); ++i) {
            const double pct = (i == raw.size() - 1) ? (100.0 - running) : base;
            out << pct;
            running += pct;
        }
        return out;
    }

    double running = 0.0;
    for (int i = 0; i < weights.size(); ++i) {
        double pct = (weights[i] / sum) * 100.0;
        if (i == weights.size() - 1)
            pct = 100.0 - running;
        out << pct;
        running += pct;
    }
    return out;
}

static QList<double> displayPercentValues(const QList<double>& exact)
{
    QList<double> out;
    if (exact.isEmpty()) return out;

    QVector<double> remainders;
    remainders.reserve(exact.size());
    QVector<int> order(exact.size());
    int allocated = 0;
    for (int i = 0; i < exact.size(); ++i) {
        const double scaled = qMax(0.0, exact[i]) * 10.0;
        const int base = int(std::floor(scaled + 1e-9));
        out << (base / 10.0);
        remainders << (scaled - base);
        order[i] = i;
        allocated += base;
    }

    int remaining = qMax(0, 1000 - allocated);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        if (qFuzzyCompare(remainders[a] + 1.0, remainders[b] + 1.0))
            return a < b;
        return remainders[a] > remainders[b];
    });
    for (int i = 0; i < remaining; ++i) {
        const int idx = order[i % order.size()];
        out[idx] += 0.1;
    }
    return out;
}

static void buildComparePieSlices(const QStringList& names,
                                  const QList<double>& totals,
                                  int baseIdx,
                                  QStringList& outLabels,
                                  QList<double>& outValues)
{
    outLabels.clear();
    outValues.clear();

    const auto appendTotalMode = [&]() {
        const QList<double> normalized = normalizePercentValues(totals);
        for (int i = 0; i < normalized.size(); ++i) {
            outLabels << names.value(i, QStringLiteral("Series %1").arg(i + 1));
            outValues << normalized.value(i);
        }
    };

    if (totals.isEmpty())
        return;

    if (baseIdx < 0 || baseIdx >= totals.size()) {
        appendTotalMode();
        return;
    }

    const double baseValue = qAbs(totals.value(baseIdx));
    QList<double> components;
    QStringList componentLabels;
    for (int i = 0; i < totals.size(); ++i) {
        if (i == baseIdx)
            continue;
        components << qAbs(totals.value(i));
        componentLabels << names.value(i, QStringLiteral("Series %1").arg(i + 1));
    }

    if (components.isEmpty() || baseValue < 0.0001) {
        appendTotalMode();
        return;
    }

    double sumComponents = 0.0;
    for (double v : components) sumComponents += v;

    QList<double> percentValues;
    percentValues.reserve(components.size() + 1);
    QStringList labels;

    if (sumComponents <= baseValue + 0.0001) {
        double running = 0.0;
        for (double v : components) {
            const double pct = (v / baseValue) * 100.0;
            percentValues << pct;
            running += pct;
        }
        labels = componentLabels;
        labels << names.value(baseIdx, QStringLiteral("Series %1").arg(baseIdx + 1));
        percentValues << qMax(0.0, 100.0 - running);
    } else {
        // The selected non-base metrics exceed the chosen base.
        // Keep the full set on the chart and normalize the slices to 100%.
        labels = componentLabels;
        labels << names.value(baseIdx, QStringLiteral("Series %1").arg(baseIdx + 1));
        percentValues = normalizePercentValues(components);
        percentValues << 0.0;
    }

    outLabels = labels;
    outValues = normalizePercentValues(percentValues);
}

static const char* kResultsSSDark = R"(
QWidget#resultsRoot { background:#0d1020; }
QWidget#summaryBar {
    background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #131729, stop:1 #0d1020);
    border-bottom:1px solid #1e2445;
}
QWidget#modeBanner {
    background:#171c33;
    border-bottom:1px solid #252b52;
}
QLabel#modeText {
    color:#c8d0ed; font-weight:800; letter-spacing:1px; background:transparent;
}
QWidget#sumCard {
    background:#1a1f38;
    border-radius:10px;
    border:1px solid #252b52;
    padding:14px 22px;
}
QLabel#sumTitle {
    color:#5a6490; font-weight:700;
    letter-spacing:1px; background:transparent;
}
QLabel#sumValue { font-weight:900; background:transparent; }
QToolButton#hiddenChartsBtn, QToolButton#monthSelectBtn, QToolButton#pageModeBtn {
    background:#1a1f38; color:#c8d0ed; border:1px solid #252b52;
    border-radius:8px; padding:8px 14px; font-weight:700;
}
QToolButton#hiddenChartsBtn:hover, QToolButton#monthSelectBtn:hover, QToolButton#pageModeBtn:hover { background:#1e2445; }
QWidget#gridContainer { background:#0d1020; }
QWidget#reportSection, QWidget#pageBreakSection, QWidget#chartsSection { background:transparent; }
QFrame#pageBreakLine { background:#252b52; min-height:1px; max-height:1px; border:none; }
QLabel#pageBreakLabel { color:#5a6490; font-weight:800; letter-spacing:1px; background:transparent; }
QLabel#sectionTitle { color:#c8d0ed; font-weight:800; }
QLabel#sectionSub { color:#5a6490; }
QTableWidget {
    background:#111526; color:#c8d0ed; gridline-color:#252b52;
    border:1px solid #252b52; border-radius:10px;
}
QHeaderView::section {
    background:#1a1f38; color:#4f86f7; border:none; padding:8px 10px;
    font-weight:700;
}
QTableWidget::item { padding:6px; }
QTableWidget::item:selected { background:#1e2445; }
QLabel#emptyMsg { color:#2e3860; font-weight:700; }
)";

static const char* kResultsSSLight = R"(
QWidget#resultsRoot { background:#f4f6fb; }
QWidget#summaryBar {
    background:qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #f4f6fb);
    border-bottom:1px solid #dde2f0;
}
QWidget#modeBanner {
    background:#ffffff;
    border-bottom:1px solid #dde2f0;
}
QLabel#modeText {
    color:#1e2340; font-weight:800; letter-spacing:1px; background:transparent;
}
QWidget#sumCard {
    background:#ffffff; border-radius:10px; border:1px solid #dde2f0;
    padding:14px 22px;
}
QLabel#sumTitle {
    color:#8892b8; font-weight:700;
    letter-spacing:1px; background:transparent;
}
QLabel#sumValue { font-weight:900; background:transparent; }
QToolButton#hiddenChartsBtn, QToolButton#monthSelectBtn, QToolButton#pageModeBtn {
    background:#ffffff; color:#1e2340; border:1px solid #dde2f0;
    border-radius:8px; padding:8px 14px; font-weight:700;
}
QToolButton#hiddenChartsBtn:hover, QToolButton#monthSelectBtn:hover, QToolButton#pageModeBtn:hover { background:#eef0fa; }
QWidget#gridContainer { background:#f4f6fb; }
QWidget#reportSection, QWidget#pageBreakSection, QWidget#chartsSection { background:transparent; }
QFrame#pageBreakLine { background:#dde2f0; min-height:1px; max-height:1px; border:none; }
QLabel#pageBreakLabel { color:#8892b8; font-weight:800; letter-spacing:1px; background:transparent; }
QLabel#sectionTitle { color:#1e2340; font-weight:800; }
QLabel#sectionSub { color:#6b7280; }
QTableWidget {
    background:#ffffff; color:#1e2340; gridline-color:#dde2f0;
    border:1px solid #dde2f0; border-radius:10px;
}
QHeaderView::section {
    background:#f6f8fe; color:#4f86f7; border:none; padding:8px 10px;
    font-weight:700;
}
QTableWidget::item { padding:6px; }
QTableWidget::item:selected { background:#eef0fa; }
QLabel#emptyMsg { color:#8aa0c8; font-weight:700; }
)";

static QString themedPopupMenuStyle()
{
    return g_lightMode
        ? QStringLiteral(
              "QMenu{background:#ffffff;color:#1e2340;border:1px solid #d9e0ef;padding:4px;}"
              "QMenu::item{padding:7px 22px;min-width:180px;}"
              "QMenu::item:selected{background:#eef0fa;color:#1e2340;}"
              "QMenu::separator{height:1px;background:#e5e8f2;margin:4px 6px;}")
        : QStringLiteral(
              "QMenu{background:#1a1f38;color:#e7ecff;border:1px solid #343c63;padding:4px;}"
              "QMenu::item{padding:7px 22px;min-width:180px;}"
              "QMenu::item:selected{background:#4f86f7;color:#ffffff;}"
              "QMenu::separator{height:1px;background:#2b3257;margin:4px 6px;}");
}

static void applyPopupMenuStyle(QMenu* menu)
{
    if (!menu) return;
    menu->setCursor(Qt::PointingHandCursor);
    menu->setStyleSheet(themedPopupMenuStyle());
}

static bool sameChartRequest(const ChartRequest& a, const ChartRequest& b)
{
    return a.kind == b.kind
        && a.metricA == b.metricA
        && a.metricB == b.metricB
        && a.compareMetrics == b.compareMetrics
        && a.comparePieBaseMetric == b.comparePieBaseMetric
        && a.title == b.title
        && a.seriesA == b.seriesA
        && a.seriesB == b.seriesB
        && a.months == b.months
        && a.accountFilter == b.accountFilter;
}

static QString money(double v)
{
    return QString("$%L1").arg(v, 0, 'f', 0);
}

static void setTableRow(QTableWidget* t, int row, const QString& month, double net, double cogs, double profit)
{
    auto mkItem = [](const QString& text, const QColor& color = {}) {
        auto* it = new QTableWidgetItem(text);
        if (color.isValid()) it->setForeground(color);
        it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        return it;
    };

    t->setItem(row, 0, mkItem(month));
    t->setItem(row, 1, mkItem(money(net), net >= 0 ? QColor("#3ecf8e") : QColor("#e05c6a")));
    t->setItem(row, 2, mkItem(money(cogs), QColor("#f0a500")));
    t->setItem(row, 3, mkItem(money(profit), profit >= 0 ? QColor("#3ecf8e") : QColor("#e05c6a")));
}

static void applyChartAxes(QChart* chart, const QStringList& labels)
{
    auto* axisX = new QBarCategoryAxis;
    axisX->append(labels);
    axisX->setLabelsFont(QFont("Segoe UI", 8));
    axisX->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisX->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    chart->addAxis(axisX, Qt::AlignBottom);
}

static QStringList monthlyLabels()
{
    return monthNames();
}

static QString pageModeText(bool landscape)
{
    return landscape ? tr_landscape_94f6c5() : tr_portrait_247c2f();
}

static QString percentText(double pct)
{
    return QString::number(pct, 'f', 1) + QStringLiteral("%");
}

static QList<double> toDoubleList(const QVariant& v)
{
    QList<double> out;
    const QVariantList list = v.toList();
    out.reserve(list.size());
    for (const QVariant& x : list) out << x.toDouble();
    return out;
}

static QList<QList<double>> toDoubleLists(const QVariant& v)
{
    QList<QList<double>> out;
    const QVariantList outer = v.toList();
    out.reserve(outer.size());
    for (const QVariant& one : outer) out << toDoubleList(one);
    return out;
}

static QList<double> computePercentages(const QList<double>& values)
{
    QList<double> out;
    out.reserve(values.size());
    double total = 0.0;
    for (double v : values) total += qAbs(v);
    if (total < 0.000001) {
        for (int i = 0; i < values.size(); ++i) out << 0.0;
        return out;
    }
    for (double v : values) out << (qAbs(v) / total) * 100.0;
    return out;
}

static QList<double> computePercentagesAgainstBase(const QList<double>& values, const QList<double>& base)
{
    QList<double> out;
    const int n = qMax(values.size(), base.size());
    out.reserve(n);
    for (int i = 0; i < n; ++i) {
        const double v = i < values.size() ? values[i] : 0.0;
        const double b = i < base.size() ? qAbs(base[i]) : 0.0;
        out << (b < 0.000001 ? 0.0 : (qAbs(v) / b) * 100.0);
    }
    return out;
}

static double computeTotalPercentageAgainstBase(const QList<double>& values, const QList<double>& base)
{
    double totalV = 0.0, totalB = 0.0;
    for (double v : values) totalV += qAbs(v);
    for (double b : base) totalB += qAbs(b);
    return totalB < 0.000001 ? 0.0 : (totalV / totalB) * 100.0;
}

static double totalAbsValue(const QList<double>& values)
{
    double total = 0.0;
    for (double v : values) total += qAbs(v);
    return total;
}

static void drawPercentText(QPainter& p, const QPointF& pos, const QString& text, const QColor& fg, const QColor& bg)
{
    const QRectF r(pos.x() - 22.0, pos.y() - 10.0, 44.0, 18.0);
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRoundedRect(r, 6.0, 6.0);
    p.setPen(fg);
    p.drawText(r, Qt::AlignCenter, text);
}

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

    void paintEvent(QPaintEvent* event) override
    {
        QChartView::paintEvent(event);
        QChart* c = chart();
        if (!c) return;

        const QString type = property("chartType").toString();
        const QColor fg = g_lightMode ? QColor("#111827") : QColor("#f9fafb");
        const QColor bg = g_lightMode ? QColor(255,255,255,215) : QColor(17,24,39,215);
        QPainter p(viewport());
        p.setRenderHint(QPainter::Antialiasing);
        p.setFont(QFont("Segoe UI", 8, QFont::Bold));

        auto placeY = [&](double value, double maxAbs) {
            const QRectF plot = c->plotArea();
            if (maxAbs < 0.000001) maxAbs = 1.0;
            const double frac = qBound(0.0, value / (maxAbs * 1.1), 1.0);
            return plot.bottom() - frac * plot.height();
        };

        if (type == "line") {
            const QList<double> values = toDoubleList(property("chartValues"));
            const QList<double> pcts = toDoubleList(property("chartPercents"));
            if (values.isEmpty() || pcts.size() != values.size() || c->series().isEmpty()) return;
            auto* line = qobject_cast<QLineSeries*>(c->series().first());
            if (!line) return;
            for (int i = 0; i < pcts.size() && i < line->count(); ++i)
                drawPercentText(p, mapFromScene(c->mapToPosition(line->at(i), line)) + QPoint(0, -14), percentText(pcts[i]), fg, bg);
            return;
        }

        if (type == "linecompare") {
            const QList<QList<double>> pcts = toDoubleLists(property("chartPercentsMulti"));
            if (pcts.isEmpty()) return;
            const auto series = c->series();
            for (int s = 0; s < pcts.size() && s < series.size(); ++s) {
                auto* line = qobject_cast<QLineSeries*>(series[s]);
                if (!line) continue;
                for (int i = 0; i < pcts[s].size() && i < line->count(); ++i)
                    drawPercentText(p, mapFromScene(c->mapToPosition(line->at(i), line)) + QPoint(0, -14), percentText(pcts[s][i]), fg, bg);
            }
            return;
        }

        if (type == "rankedbar" || type == "candle") {
            const QList<double> values = toDoubleList(property("chartValues"));
            const QList<double> pcts = toDoubleList(property("chartPercents"));
            if (values.isEmpty() || pcts.size() != values.size()) return;
            double maxAbs = 0.0; for (double v : values) maxAbs = qMax(maxAbs, qAbs(v));
            const QRectF plot = c->plotArea();
            const double slot = plot.width() / qMax(1, values.size());
            for (int i = 0; i < values.size(); ++i) {
                const double x = plot.left() + slot * (i + 0.5);
                const double y = placeY(qAbs(values[i]), maxAbs) - 14.0;
                drawPercentText(p, QPointF(x, y), percentText(pcts[i]), fg, bg);
            }
            return;
        }

        if (type == "barcompare" || type == "comparecandle") {
            const QList<QList<double>> values = property("chartValuesMulti").isValid() ? toDoubleLists(property("chartValuesMulti"))
                                                                                         : QList<QList<double>>{toDoubleList(property("chartValues")), toDoubleList(property("chartValues2"))};
            const QList<QList<double>> pcts = toDoubleLists(property("chartPercentsMulti"));
            if (values.isEmpty() || pcts.size() != values.size()) return;
            double maxAbs = 0.0;
            int count = 0;
            for (const auto& one : values) {
                count = qMax(count, one.size());
                for (double v : one) maxAbs = qMax(maxAbs, qAbs(v));
            }
            const QRectF plot = c->plotArea();
            const double slot = plot.width() / qMax(1, count);
            const double gap = qMin(28.0, slot * 0.6);
            const int seriesCount = values.size();
            for (int s = 0; s < values.size(); ++s) {
                for (int i = 0; i < values[s].size() && i < pcts[s].size(); ++i) {
                    const double offset = ((s + 0.5) / qMax(1, seriesCount) - 0.5) * gap;
                    const double x = plot.left() + slot * (i + 0.5) + offset;
                    const double y = placeY(qAbs(values[s][i]), maxAbs) - 14.0;
                    drawPercentText(p, QPointF(x, y), percentText(pcts[s][i]), fg, bg);
                }
            }
        }
    }
};

static QChartView* makeChartView(QChart* chart, bool zoomable = false)
{
    auto* view = new SafeChartView(chart);
    if (zoomable) {
        // intentionally left disabled; charts remain fixed and non-interactive
    }
    return view;
}


static const char* kMonthCardSSDark = R"(
QFrame#monthCard {
    background:#1a1f38;
    border:1px solid #252b52;
    border-radius:12px;
}
QFrame#monthCard[highlighted="true"] {
    border:2px solid #4f86f7;
}
QWidget#monthHandle {
    background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #1e264a, stop:1 #1a1f38);
    border-bottom:1px solid #252b52;
    border-radius:12px 12px 0 0;
}
QLabel#monthTitle {
    color:#c8d0ed;
    font-weight:800;
    background:transparent;
}
QLabel#monthHint {
    color:#5a6490;
    font-weight:700;
    background:transparent;
}
QFrame#metricBox {
    background:#111526;
    border:1px solid #252b52;
    border-radius:10px;
}
QLabel#metricLabel {
    color:#5a6490;
    font-weight:700;
    background:transparent;
}
QLabel#metricValue {
    color:#c8d0ed;
    font-weight:900;
    background:transparent;
}
)";

static const char* kMonthCardSSLight = R"(
QFrame#monthCard {
    background:#ffffff;
    border:1px solid #dde2f0;
    border-radius:12px;
}
QFrame#monthCard[highlighted="true"] {
    border:2px solid #4f86f7;
}
QWidget#monthHandle {
    background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #eef0fa, stop:1 #ffffff);
    border-bottom:1px solid #dde2f0;
    border-radius:12px 12px 0 0;
}
QLabel#monthTitle {
    color:#1e2340;
    font-weight:800;
    background:transparent;
}
QLabel#monthHint {
    color:#8aa0c8;
    font-weight:700;
    background:transparent;
}
QFrame#metricBox {
    background:#f6f8fe;
    border:1px solid #dde2f0;
    border-radius:10px;
}
QLabel#metricLabel {
    color:#8892b8;
    font-weight:700;
    background:transparent;
}
QLabel#metricValue {
    color:#1e2340;
    font-weight:900;
    background:transparent;
}
)";

class MonthReportCard : public QFrame
{
    Q_OBJECT
public:
    MonthReportCard(const QString& month,
                    double netSales,
                    double cogs,
                    double profit,
                    const QColor& accent,
                    InventoryMode mode = InventoryMode::Periodic,
                    QWidget* parent = nullptr)
        : QFrame(parent), m_month(month), m_mode(mode)
    {
        setObjectName("monthCard");
        setAcceptDrops(true);
        setCursor(Qt::OpenHandCursor);
        setStyleSheet(g_lightMode ? kMonthCardSSLight : kMonthCardSSDark);
        setFixedHeight(168);
        hide();
        
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        m_handle = new QWidget;
        m_handle->setObjectName("monthHandle");
        m_handle->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        auto* hl = new QHBoxLayout(m_handle);
        hl->setContentsMargins(14, 10, 14, 10);
        hl->setSpacing(10);

        auto* dot = new QLabel("◆");
        dot->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        dot->setStyleSheet(QString("color:%1;background:transparent;font-weight:900;").arg(accent.name()));
        m_title = new QLabel(month.toUpper());
        m_title->setObjectName("monthTitle");
        m_title->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        auto* hint = new QLabel("⋮⋮");
        hint->setObjectName("monthHint");
        hint->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        hint->setStyleSheet(QString("color:%1;").arg(accent.name()));

        hl->addWidget(dot);
        hl->addWidget(m_title);
        hl->addStretch();
        hl->addWidget(hint);
        root->addWidget(m_handle);

        auto* grid = new QGridLayout;
        grid->setContentsMargins(12, 12, 12, 12);
        grid->setHorizontalSpacing(10);
        grid->setVerticalSpacing(10);

        auto makeMetric = [&](QLabel*& labelOut, int row, int col, const QString& label, const QString& value, const QColor& valueColor) {
            auto* box = new QFrame;
            box->setObjectName("metricBox");
            box->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            auto* bl = new QVBoxLayout(box);
            bl->setContentsMargins(10, 8, 10, 8);
            bl->setSpacing(4);
            labelOut = new QLabel(label);
            labelOut->setObjectName("metricLabel");
            labelOut->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            auto* val = new QLabel(value);
            val->setObjectName("metricValue");
            val->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            val->setStyleSheet(QString("color:%1;background:transparent;font-weight:900;").arg(valueColor.name()));
            bl->addWidget(labelOut);
            bl->addWidget(val);
            grid->addWidget(box, row, col);
        };

        makeMetric(m_netLabel, 0, 0, tr_net_sales_23a2f1(), money(netSales),
                   netSales >= 0 ? QColor("#3ecf8e") : QColor("#e05c6a"));
        makeMetric(m_cogsLabel, 0, 1, tr_cogs_d716f1(), money(cogs), QColor("#f0a500"));
        makeMetric(m_profitLabel, 1, 0, tr_profit_margin_ec3b22(), money(profit),
                   profit >= 0 ? QColor("#3ecf8e") : QColor("#e05c6a"));
        auto* spacer = new QFrame;
        spacer->setObjectName("metricBox");
        spacer->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        spacer->setStyleSheet("background:transparent;border:none;");
        grid->addWidget(spacer, 1, 1);

        root->addLayout(grid);

        // Make the whole card draggable by disabling child hit-testing.
        for (auto* child : findChildren<QWidget*>()) {
            child->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        }
    }

    void setCardIndex(int i) { m_index = i; }
    void setFlowIndex(int i) { m_flowIndex = i; }
    void setMonthIndex(int i) { m_monthIndex = i; }
    int cardIndex() const { return m_index; }
    QString month() const { return m_month; }
    void retranslate()
    {
        if (m_title) {
            if (m_monthIndex < 0) {
                m_title->setText(tr_all_months_e73b82());
            } else {
                m_title->setText(monthNames().value(m_monthIndex).toUpper());
            }
        }
        if (m_netLabel)    m_netLabel->setText(tr_net_sales_23a2f1());
        if (m_cogsLabel)
            m_cogsLabel->setText(tr_cogs_d716f1());
        if (m_profitLabel) m_profitLabel->setText(tr_profit_margin_ec3b22());
    }

signals:
    void swapRequested(int fromIdx, int toIdx);
    void insertSeparatorRequested(int afterFlowIndex);

protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e && e->button() == Qt::LeftButton) {
            m_dragStart = e->pos();
            e->accept();
            return;
        }
        QFrame::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if (!e || !(e->buttons() & Qt::LeftButton)) {
            QFrame::mouseMoveEvent(e);
            return;
        }
        if ((e->pos() - m_dragStart).manhattanLength() < QApplication::startDragDistance()) {
            return;
        }
        auto* mime = new QMimeData;
        mime->setData("application/x-account-flow-item", QByteArray::number(m_flowIndex));
        auto* drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->exec(Qt::MoveAction);
    }

    void dragEnterEvent(QDragEnterEvent* e) override
    {
        if (e && e->mimeData() && e->mimeData()->hasFormat("application/x-account-flow-item")) {
            e->acceptProposedAction();
            return;
        }
        QFrame::dragEnterEvent(e);
    }

    void dragMoveEvent(QDragMoveEvent* e) override
    {
        if (e && e->mimeData() && e->mimeData()->hasFormat("application/x-account-flow-item")) {
            e->acceptProposedAction();
            return;
        }
        QFrame::dragMoveEvent(e);
    }

    void dropEvent(QDropEvent* e) override
    {
        if (!e || !e->mimeData() || !e->mimeData()->hasFormat("application/x-account-flow-item")) {
            QFrame::dropEvent(e);
            return;
        }
        const int from = QString::fromUtf8(e->mimeData()->data("application/x-account-flow-item")).toInt();
        if (from != m_flowIndex) emit swapRequested(from, m_flowIndex);
        e->acceptProposedAction();
    }

    void contextMenuEvent(QContextMenuEvent* e) override
    {
        QMenu menu(this);
        applyPopupMenuStyle(&menu);
        QAction* insertSep = menu.addAction(tr_add_page_separator_below_862284());
        if (menu.exec(e->globalPos()) == insertSep) {
            emit insertSeparatorRequested(m_flowIndex);
        }
    }

private:
    int m_index{0};
    int m_flowIndex{0};
    QPoint m_dragStart;
    QWidget* m_handle{nullptr};
    QLabel* m_title{nullptr};
    QString m_month;
    int m_monthIndex{-1};
    QLabel* m_netLabel{nullptr};
    QLabel* m_cogsLabel{nullptr};
    QLabel* m_profitLabel{nullptr};
    InventoryMode m_mode{InventoryMode::Periodic};
};

class PageSeparatorCard : public QFrame
{
    Q_OBJECT
public:
    explicit PageSeparatorCard(QWidget* parent = nullptr) : QFrame(parent)
    {
        setObjectName("pageSeparatorCard");
        setAcceptDrops(true);
        setCursor(Qt::OpenHandCursor);
        setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
        setFixedHeight(44);

        auto* root = new QHBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        auto* left = new QFrame;
        left->setObjectName("pageBreakLine");
        left->setFrameShape(QFrame::HLine);
        left->setFrameShadow(QFrame::Plain);
        left->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* right = new QFrame;
        right->setObjectName("pageBreakLine");
        right->setFrameShape(QFrame::HLine);
        right->setFrameShadow(QFrame::Plain);
        right->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        m_label = new QLabel(tr_page_separator_5ac5db());
        m_label->setObjectName("pageBreakLabel");
        m_label->setAlignment(Qt::AlignCenter);

        root->addWidget(left, 1);
        root->addWidget(m_label);
        root->addWidget(right, 1);
    }

    void setFlowIndex(int i) { m_flowIndex = i; }
    void setSeparatorId(int id) { m_separatorId = id; }

signals:
    void swapRequested(int fromIdx, int toIdx);
    void removeRequested(int separatorId);

protected:
    void mousePressEvent(QMouseEvent* e) override
    {
        if (e && e->button() == Qt::LeftButton) {
            m_dragStart = e->pos();
            e->accept();
            return;
        }
        QFrame::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent* e) override
    {
        if (!e || !(e->buttons() & Qt::LeftButton)) {
            QFrame::mouseMoveEvent(e);
            return;
        }
        if ((e->pos() - m_dragStart).manhattanLength() < QApplication::startDragDistance()) return;
        auto* mime = new QMimeData;
        mime->setData("application/x-account-flow-item", QByteArray::number(m_flowIndex));
        auto* drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->exec(Qt::MoveAction);
    }

    void dragEnterEvent(QDragEnterEvent* e) override
    {
        if (e && e->mimeData() && e->mimeData()->hasFormat("application/x-account-flow-item")) {
            e->acceptProposedAction();
            return;
        }
        QFrame::dragEnterEvent(e);
    }

    void dragMoveEvent(QDragMoveEvent* e) override
    {
        if (e && e->mimeData() && e->mimeData()->hasFormat("application/x-account-flow-item")) {
            e->acceptProposedAction();
            return;
        }
        QFrame::dragMoveEvent(e);
    }

    void dropEvent(QDropEvent* e) override
    {
        if (!e || !e->mimeData() || !e->mimeData()->hasFormat("application/x-account-flow-item")) {
            QFrame::dropEvent(e);
            return;
        }
        const int from = QString::fromUtf8(e->mimeData()->data("application/x-account-flow-item")).toInt();
        if (from != m_flowIndex) emit swapRequested(from, m_flowIndex);
        e->acceptProposedAction();
    }

    void contextMenuEvent(QContextMenuEvent* e) override
    {
        QMenu menu(this);
        applyPopupMenuStyle(&menu);
        QAction* removeAct = menu.addAction(tr_remove_page_separator_f78ac7());
        if (menu.exec(e->globalPos()) == removeAct) emit removeRequested(m_separatorId);
    }

private:
    int m_flowIndex{0};
    int m_separatorId{-1};
    QPoint m_dragStart;
    QLabel* m_label{nullptr};
};


ResultsWidget::ResultsWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("resultsRoot");
    setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_summaryBar = new QWidget;
    m_summaryBar->setObjectName("summaryBar");
    m_summaryBar->setFixedHeight(104);
    auto* bl = new QHBoxLayout(m_summaryBar);
    bl->setContentsMargins(24, 12, 24, 12);
    bl->setSpacing(16);

    auto makeCard = [&](QLabel*& titleOut, QLabel*& valOut, const QString& txt, const QColor& col) {
        auto* card = new QWidget;
        card->setObjectName("sumCard");
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(14, 10, 14, 10);
        cl->setSpacing(4);
        titleOut = new QLabel(txt);
        titleOut->setObjectName("sumTitle");
        valOut = new QLabel("—");
        valOut->setObjectName("sumValue");
        valOut->setStyleSheet(QString("color:%1;background:transparent;").arg(col.name()));
        cl->addWidget(titleOut);
        cl->addWidget(valOut);
        bl->addWidget(card, 1);
    };

    makeCard(m_sumNetSalesTitle, m_sumNetSales, tr_net_sales_e81e65(), QColor("#3ecf8e"));
    {
        // Create the COGS card and remember its container widget so we can
        // hide it in Ongoing inventory mode.
        auto* cogsCard = new QWidget;
        cogsCard->setObjectName("sumCard");
        auto* cl = new QVBoxLayout(cogsCard);
        cl->setContentsMargins(14, 10, 14, 10);
        cl->setSpacing(4);
        m_sumCOGSTitle = new QLabel(tr_cogs_d716f1());
        m_sumCOGSTitle->setObjectName("sumTitle");
        m_sumCOGS = new QLabel("—");
        m_sumCOGS->setObjectName("sumValue");
        m_sumCOGS->setStyleSheet(QString("color:%1;background:transparent;").arg(QColor("#f0a500").name()));
        cl->addWidget(m_sumCOGSTitle);
        cl->addWidget(m_sumCOGS);
        bl->addWidget(cogsCard, 1);
        m_sumCOGSCard = cogsCard;
    }
    makeCard(m_sumProfitTitle, m_sumProfit, tr_profit_margin_dafda2(), QColor("#4f86f7"));


    m_hiddenBtn = new QToolButton;
    m_hiddenBtn->setObjectName("hiddenChartsBtn");
    m_hiddenBtn->setText(tr_hidden_charts_e4bae7());
    m_hiddenBtn->setPopupMode(QToolButton::InstantPopup);
    m_hiddenMenu = new QMenu(m_hiddenBtn);
    connect(m_hiddenMenu, &QMenu::triggered, this, &ResultsWidget::onRestoreHidden);
    m_hiddenBtn->setMenu(m_hiddenMenu);
    bl->addWidget(m_hiddenBtn, 0, Qt::AlignVCenter);

    m_monthBtn = new QToolButton;
    m_monthBtn->setObjectName("monthSelectBtn");
    m_monthBtn->setPopupMode(QToolButton::InstantPopup);
    m_monthBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_monthBtn->setArrowType(Qt::DownArrow);
    m_monthMenu = new StayOpenMenu(m_monthBtn);
    m_monthBtn->setMenu(m_monthMenu);
    bl->addWidget(m_monthBtn, 0, Qt::AlignVCenter);

    m_orientBtn = new QToolButton;
    m_orientBtn->setObjectName("pageModeBtn");
    m_orientBtn->setPopupMode(QToolButton::InstantPopup);
    m_orientBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_orientBtn->setArrowType(Qt::DownArrow);
    m_orientMenu = new QMenu(m_orientBtn);
    QAction* landscapeAct = m_orientMenu->addAction(tr_landscape_94f6c5());
    landscapeAct->setData(true);
    QAction* portraitAct = m_orientMenu->addAction(tr_portrait_247c2f());
    portraitAct->setData(false);
    connect(m_orientMenu, &QMenu::triggered, this, [this](QAction* act) {
        if (!act) return;
        m_pageLandscape = act->data().toBool();
        updatePageMode();
    });
    m_orientBtn->setMenu(m_orientMenu);
    bl->addWidget(m_orientBtn, 0, Qt::AlignVCenter);

    root->addWidget(m_summaryBar);

    m_modeBanner = new QWidget;
    m_modeBanner->setObjectName("modeBanner");
    m_modeBanner->setFixedHeight(40);
    auto* mb = new QHBoxLayout(m_modeBanner);
    mb->setContentsMargins(24, 0, 24, 0);
    m_modeLabel = new QLabel;
    m_modeLabel->setObjectName("modeText");
    mb->addWidget(m_modeLabel);
    mb->addStretch();
    root->addWidget(m_modeBanner);

    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setStyleSheet("QScrollArea{background:transparent;}QScrollArea QWidget{background:transparent;}");

    m_container = new QWidget;
    m_container->setObjectName("gridContainer");
    m_contentLayout = new QVBoxLayout(m_container);
    m_contentLayout->setContentsMargins(20, 20, 20, 20);
    m_contentLayout->setSpacing(18);
    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll, 1);

    rebuildHiddenMenu();
    rebuildMonthSelectorMenu();
    updatePageMode();
}


void ResultsWidget::appendChart(const AppData& data, const ChartRequest& request)
{
    if (!m_flowSection || !m_flowLayout)
        return;

    QChartView* view = createChartView(data, request);
    if (!view)
        return;

    addCard(request, view);

    ResultFlowItem item;
    item.kind = ResultFlowItemKind::ChartCard;
    item.index = m_cards.isEmpty() ? -1 : m_cards.last()->cardIndex();
    m_flowOrder.append(item);

    rebuildFlow();
    rebuildHiddenMenu();
    emit resultsStateChanged();
}

void ResultsWidget::clearResults()
{
    if (m_flowSection) {
        m_contentLayout->removeWidget(m_flowSection);
        m_flowSection->deleteLater();
        m_flowSection = nullptr;
        m_flowLayout = nullptr;
        m_flowEmpty = nullptr;
    }

    if (m_monthSection) {
        m_contentLayout->removeWidget(m_monthSection);
        m_monthSection->deleteLater();
        m_monthSection = nullptr;
        m_monthGrid = nullptr;
        m_monthEmpty = nullptr;
    }
    for (auto* c : m_monthCards) c->deleteLater();
    m_monthCards.clear();
    m_monthOrder.clear();
    m_visibleMonths.clear();

    if (m_pageBreakSection) {
        m_contentLayout->removeWidget(m_pageBreakSection);
        m_pageBreakSection->deleteLater();
        m_pageBreakSection = nullptr;
    }
    if (m_chartsSection) {
        m_contentLayout->removeWidget(m_chartsSection);
        m_chartsSection->deleteLater();
        m_chartsSection = nullptr;
        m_grid = nullptr;
        m_emptyState = nullptr;
    }
    for (auto* c : m_cards) c->deleteLater();
    for (auto* c : m_hiddenCards) c->deleteLater();
    m_cards.clear();
    m_cardRequests.clear();
    m_hiddenCards.clear();
    m_hiddenRequests.clear();
    m_separatorCards.clear();
    m_flowOrder.clear();
    rebuildHiddenMenu();
}

void ResultsWidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_summaryBar) m_summaryBar->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_pageBreakSection) m_pageBreakSection->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_hiddenBtn) m_hiddenBtn->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_monthBtn) m_monthBtn->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_orientBtn) m_orientBtn->setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    if (m_scroll) m_scroll->setStyleSheet(g_lightMode
        ? "QScrollArea{background:#f4f6fb;}QScrollArea QWidget{background:transparent;}QScrollBar:vertical{background:#f4f6fb;width:8px;border-radius:4px;}QScrollBar::handle:vertical{background:#c8d0ed;border-radius:4px;min-height:30px;}QScrollBar::handle:vertical:hover{background:#4f86f7;}"
        : "QScrollArea{background:#0d1020;}QScrollArea QWidget{background:transparent;}QScrollBar:vertical{background:#0d1020;width:8px;border-radius:4px;}QScrollBar::handle:vertical{background:#2e3860;border-radius:4px;min-height:30px;}QScrollBar::handle:vertical:hover{background:#4f86f7;}");
    for (auto* card : m_monthCards) {
        if (card) card->setStyleSheet(g_lightMode ? kMonthCardSSLight : kMonthCardSSDark);
    }
}

void ResultsWidget::retranslate()
{
    if (m_sumNetSalesTitle) m_sumNetSalesTitle->setText(tr_net_sales_e81e65());
    if (m_sumCOGSTitle)     m_sumCOGSTitle->setText(tr_cogs_d716f1());
    if (m_sumProfitTitle)   m_sumProfitTitle->setText(tr_profit_margin_dafda2());
    if (m_sumCOGSCard)      m_sumCOGSCard->setVisible(true);

    if (m_hiddenBtn) m_hiddenBtn->setText(tr_hidden_charts_7e1497());

    if (m_orientMenu) {
        m_orientMenu->disconnect();
        m_orientMenu->clear();
        QAction* landscapeAct = m_orientMenu->addAction(tr_landscape_94f6c5());
        landscapeAct->setData(true);
        QAction* portraitAct = m_orientMenu->addAction(tr_portrait_247c2f());
        portraitAct->setData(false);
        connect(m_orientMenu, &QMenu::triggered, this, [this](QAction* act) {
            if (!act) return;
            m_pageLandscape = act->data().toBool();
            updatePageMode();
        });
    }

    if (m_reportTitle) m_reportTitle->setText(tr_monthly_report_18dfcd());
    if (m_reportSub) m_reportSub->setText(m_lastMode == InventoryMode::Ongoing
                                           ? tr_ongoing_inventory_4f9f2c() + QStringLiteral(" — ") + tr_each_month_appears_as_a_dragga_9d0352()
                                           : tr_each_month_appears_as_a_dragga_9d0352());
    if (m_pageBreakLabel) m_pageBreakLabel->setText(tr_page_break_0e9502());
    if (m_chartsTitle) m_chartsTitle->setText(tr_charts_ced4c1());
    if (m_chartsSub) m_chartsSub->setText(tr_drag_to_reorder_right_click_a__b70e11());
    if (m_modeLabel) m_modeLabel->setText(m_lastMode == InventoryMode::Ongoing ? tr_ongoing_inventory_4f9f2c() : tr_periodic_inventory_8a4f19());
    if (m_flowTitle) m_flowTitle->setText(tr_results_page_3159bf());
    if (m_flowSub) m_flowSub->setText(m_lastMode == InventoryMode::Ongoing
                                       ? tr_ongoing_inventory_4f9f2c() + QStringLiteral(" — ") + tr_choose_one_or_more_month_cards_18cee3()
                                       : tr_choose_one_or_more_month_cards_18cee3());
    if (m_monthEmpty) m_monthEmpty->setText(tr_no_months_available_9220b0());
    if (m_emptyState) m_emptyState->setText(tr_no_charts_selected_7a4c8f());
    if (m_flowEmpty) m_flowEmpty->setText(tr_no_results_available_669e79());

    for (auto* card : findChildren<MonthReportCard*>()) {
        if (card) card->retranslate();
    }

    rebuildHiddenMenu();
    rebuildMonthSelectorMenu();
    updatePageMode();
}


void ResultsWidget::buildResults(const AppData& data)
{
    setStyleSheet(g_lightMode ? kResultsSSLight : kResultsSSDark);
    m_lastMode = data.inventoryMode;
    if (m_modeLabel) m_modeLabel->setText(m_lastMode == InventoryMode::Ongoing ? tr_ongoing_inventory_4f9f2c() : tr_periodic_inventory_8a4f19());
    if (m_sumCOGSCard) m_sumCOGSCard->setVisible(true);
    if (m_hiddenBtn) m_hiddenBtn->setEnabled(false);

    clearResults();

    m_sumNetSales->setText(money(data.totalNetSales));
    m_sumCOGS->setText(money(data.totalCOGS));
    m_sumProfit->setText(money(data.totalProfit));
    m_sumProfit->setStyleSheet(QString("color:%1;font-weight:900;background:transparent;")
                               .arg(data.totalProfit >= 0 ? "#3ecf8e" : "#e05c6a"));

    m_flowSection = new QWidget;
    m_flowSection->setObjectName("flowSection");
    auto* vl = new QVBoxLayout(m_flowSection);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(14);

    m_flowTitle = new QLabel(tr_results_page_3159bf());
    m_flowTitle->setObjectName("sectionTitle");
    m_flowSub = new QLabel(m_lastMode == InventoryMode::Ongoing ? tr_ongoing_inventory_4f9f2c() + QStringLiteral(" — ") + tr_choose_one_or_more_month_cards_18cee3() : tr_choose_one_or_more_month_cards_18cee3());
    m_flowSub->setObjectName("sectionSub");
    vl->addWidget(m_flowTitle);
    vl->addWidget(m_flowSub);

    m_flowLayout = new QVBoxLayout;
    m_flowLayout->setContentsMargins(0, 0, 0, 0);
    m_flowLayout->setSpacing(16);
    vl->addLayout(m_flowLayout);
    m_contentLayout->addWidget(m_flowSection);

    m_monthCards.clear();
    m_monthOrder.clear();
    m_cards.clear();
    m_cardRequests.clear();
    m_separatorCards.clear();
    m_flowOrder.clear();
    m_nextSeparatorId = 0;
    m_nextCardId = 0;

    static const QColor kMonthAccents[] = {
        QColor("#4f86f7"), QColor("#f0a500"), QColor("#e05c6a"), QColor("#3ecf8e"),
        QColor("#9b6cf9"), QColor("#62c4e3"), QColor("#ff9f43"), QColor("#b0e96a"),
        QColor("#fd79a8"), QColor("#00cec9"), QColor("#4f86f7"), QColor("#f0a500"),
        QColor("#3ecf8e")
    };

    m_monthCards.append(new MonthReportCard(
        tr_all_months_428b74(),
        data.totalNetSales,
        data.totalCOGS,
        data.totalProfit,
        kMonthAccents[0],
        m_lastMode,
        m_container));
    m_monthCards.last()->setCardIndex(0);
    m_monthCards.last()->setFlowIndex(0);
    m_monthCards.last()->setMonthIndex(-1);
    connect(m_monthCards.last(), &MonthReportCard::swapRequested, this, &ResultsWidget::onSwapFlowItems);
    connect(m_monthCards.last(), &MonthReportCard::insertSeparatorRequested, this, &ResultsWidget::onAddSeparatorAfter);
    m_monthOrder.append(0);

    const auto months = monthNames();
    for (int i = 0; i < 12; ++i) {
        auto* card = new MonthReportCard(
            months.value(i),
            data.netSales[i],
            data.cogs[i],
            data.profitMargin[i],
            kMonthAccents[(i + 1) % 13],
            m_lastMode,
            m_container);
        card->setCardIndex(i + 1);
        card->setFlowIndex(i + 1);
        card->setMonthIndex(i);
        connect(card, &MonthReportCard::swapRequested, this, &ResultsWidget::onSwapFlowItems);
        connect(card, &MonthReportCard::insertSeparatorRequested, this, &ResultsWidget::onAddSeparatorAfter);
        m_monthCards.append(card);
        m_monthOrder.append(i + 1);
    }

    m_visibleMonths.clear();
    for (const auto& item : data.resultFlowOrder) {
        if (item.kind == ResultFlowItemKind::MonthCard && item.index >= 0 && item.index <= 12 && !m_visibleMonths.contains(item.index))
            m_visibleMonths << item.index;
    }
    // NOTE: intentionally allow empty — means no month cards shown by default
    // User selects which months to display via the Months dropdown

    for (const auto& req : data.chartRequests) {
        QChartView* view = createChartView(data, req);
        if (!view) continue;
        addCard(req, view);
    }

    for (const auto& req : data.hiddenChartRequests) {
        QChartView* view = createChartView(data, req);
        if (!view) continue;
        addHiddenCard(req, view);
    }

    rebuildMonthSelectorMenu();
    ensureDefaultFlowOrder();
    updatePageMode();
    rebuildFlow();
    rebuildHiddenMenu();
    emit resultsStateChanged();
}


QWidget* ResultsWidget::buildReportSection(const AppData& data)
{
    auto* section = new QWidget;
    section->setObjectName("reportSection");
    auto* vl = new QVBoxLayout(section);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(16);

    m_reportTitle = new QLabel(tr_monthly_report_18dfcd());
    m_reportTitle->setObjectName("sectionTitle");
    m_reportSub = new QLabel(m_lastMode == InventoryMode::Ongoing ? tr_ongoing_inventory_4f9f2c() + QStringLiteral(" — ") + tr_each_month_appears_as_a_dragga_9d0352() : tr_each_month_appears_as_a_dragga_9d0352());
    m_reportSub->setObjectName("sectionSub");
    vl->addWidget(m_reportTitle);
    vl->addWidget(m_reportSub);

    m_monthGrid = new QGridLayout;
    m_monthGrid->setContentsMargins(0, 0, 0, 0);
    m_monthGrid->setSpacing(16);
    vl->addLayout(m_monthGrid);

    m_monthCards.clear();
    m_monthOrder.clear();

    static const QColor kMonthAccents[] = {
        QColor("#4f86f7"), QColor("#f0a500"), QColor("#e05c6a"), QColor("#3ecf8e"),
        QColor("#9b6cf9"), QColor("#62c4e3"), QColor("#ff9f43"), QColor("#b0e96a"),
        QColor("#fd79a8"), QColor("#00cec9"), QColor("#4f86f7"), QColor("#f0a500")
    };

    const auto months = monthNames();
    for (int i = 0; i < 12; ++i) {
        m_monthOrder.append(i);
        auto* card = new MonthReportCard(
            months.value(i),
            data.netSales[i],
            data.cogs[i],
            data.profitMargin[i],
            kMonthAccents[i % 12],
            m_lastMode,
            m_container);
        card->setCardIndex(i);
        card->setMonthIndex(i);
        connect(card, &MonthReportCard::swapRequested, this, &ResultsWidget::onSwapMonthCards);
        connect(card, &MonthReportCard::insertSeparatorRequested, this, &ResultsWidget::onAddSeparatorAfter);
        m_monthCards.append(card);
    }

    rebuildMonthGrid();
    return section;
}

QWidget* ResultsWidget::buildPageBreakSection()
{
    auto* section = new QWidget;
    section->setObjectName("pageBreakSection");
    auto* vl = new QVBoxLayout(section);
    vl->setContentsMargins(0, 10, 0, 10);
    vl->setSpacing(8);

    auto* line = new QFrame;
    line->setObjectName("pageBreakLine");
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Plain);
    line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_pageBreakLabel = new QLabel(tr_page_break_0e9502());
    m_pageBreakLabel->setObjectName("pageBreakLabel");
    m_pageBreakLabel->setAlignment(Qt::AlignCenter);

    vl->addWidget(line);
    vl->addWidget(m_pageBreakLabel);
    return section;
}

QWidget* ResultsWidget::buildChartsSection()
{
    auto* section = new QWidget;
    section->setObjectName("chartsSection");
    auto* vl = new QVBoxLayout(section);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(12);

    m_chartsTitle = new QLabel(tr_charts_ced4c1());
    m_chartsTitle->setObjectName("sectionTitle");
    m_chartsSub = new QLabel(tr_drag_to_reorder_right_click_a__b70e11());
    m_chartsSub->setObjectName("sectionSub");
    vl->addWidget(m_chartsTitle);
    vl->addWidget(m_chartsSub);

    auto* container = new QWidget;
    m_grid = new QGridLayout(container);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(16);
    vl->addWidget(container);

    return section;
}


QList<int> ResultsWidget::monthOrder() const
{
    return m_visibleMonths;
}

QList<ResultFlowItem> ResultsWidget::flowOrder() const
{
    QList<ResultFlowItem> list;
    for (const auto& item : m_flowOrder) list << item;
    return list;
}

void ResultsWidget::rebuildMonthSelectorMenu()
{
    if (!m_monthMenu || !m_monthBtn) return;
    m_monthMenu->clear();

    const auto months = monthNames();

    // ── Select All / Deselect All ──────────────────────────────────────────
    auto* selAll   = m_monthMenu->addAction(tr_select_all_7812c3());
    auto* deselAll = m_monthMenu->addAction(tr_deselect_all_474bc1());
    m_monthMenu->addSeparator();

    // ── All-months summary card (index 0) + individual months (1–12) ──────
    QList<QAction*> acts;
    for (int i = 0; i <= 12; ++i) {
        const QString label = (i == 0)
            ? tr_all_months_summary_b46139()
            : months.value(i - 1);
        QAction* act = m_monthMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_visibleMonths.contains(i));
        act->setData(i);
        acts << act;
    }

    // Helper: rebuild button label from current check state
    auto updateLabel = [this, acts]() {
        QList<int> sel;
        for (auto* a : acts)
            if (a->isChecked()) sel << a->data().toInt();

        QString txt;
        if (sel.isEmpty()) {
            txt = tr_months_none_selected_7918be();
        } else if (sel.size() == 13) {
            txt = tr_months_all_9f9e09();
        } else {
            // Build a readable label (skip index-0 "All months" summary in the name list)
            QStringList names;
            for (int x : sel) {
                names << (x == 0 ? tr_summary_04237c() : monthNames().value(x - 1));
            }
            txt = (names.size() <= 3)
                ? tr_months_d113f0() + names.join(", ")
                : tr_months_d113f0() + names.mid(0, 3).join(", ")
                  + QStringLiteral(" +%1").arg(names.size() - 3);
        }
        m_monthBtn->setText(txt);
    };

    // Helper: apply checked state → m_visibleMonths → rebuild flow
    auto applySelection = [this, acts, updateLabel]() {
        m_visibleMonths.clear();
        for (auto* a : acts)
            if (a->isChecked()) m_visibleMonths << a->data().toInt();
        updateLabel();
        ensureDefaultFlowOrder();
        rebuildFlow();
    };

    // Select All
    QObject::connect(selAll, &QAction::triggered, m_monthBtn, [acts, applySelection]() {
        for (auto* a : acts) a->setChecked(true);
        applySelection();
    });

    // Deselect All
    QObject::connect(deselAll, &QAction::triggered, m_monthBtn, [acts, applySelection]() {
        for (auto* a : acts) a->setChecked(false);
        applySelection();
    });

    // Individual month toggles
    for (auto* act : acts) {
        QObject::connect(act, &QAction::toggled, m_monthBtn, [applySelection](bool) {
            applySelection();
        });
    }

    // Set initial label
    updateLabel();
}

void ResultsWidget::setVisibleMonths(const QList<int>& months)
{
    m_visibleMonths.clear();
    for (int m : months) {
        if (m < 0 || m > 12) continue;   // 0=summary, 1-12=individual months
        if (!m_visibleMonths.contains(m)) m_visibleMonths << m;
    }
    // Allow empty — shows no month cards; user picks via dropdown
    rebuildMonthSelectorMenu();
    ensureDefaultFlowOrder();
    rebuildFlow();
}

void ResultsWidget::updatePageMode()
{
    const int width = m_pageLandscape ? 1123 : 794;
    if (m_container) {
        m_container->setMinimumWidth(width);
        m_container->setMaximumWidth(width);
    }
    if (m_orientBtn) {
        m_orientBtn->setText(tr_page_1_d40a68().arg(pageModeText(m_pageLandscape)));
    }
}

void ResultsWidget::ensureDefaultFlowOrder()
{
    // m_visibleMonths may be empty — that's valid (no month cards shown)
    // Values: 0 = "All months" summary card, 1-12 = individual month cards

    QSet<int> seenMonths;
    m_flowOrder.clear();
    for (int idx : m_visibleMonths) {
        if (idx < 0 || idx > 12 || seenMonths.contains(idx)) continue;
        seenMonths.insert(idx);
        m_flowOrder.append(ResultFlowItem{ResultFlowItemKind::MonthCard, idx, -1});
    }
    for (auto* card : m_cards) {
        if (!card) continue;
        m_flowOrder.append(ResultFlowItem{ResultFlowItemKind::ChartCard, card->cardIndex(), -1});
    }
}


void ResultsWidget::rebuildMonthGrid()
{
    if (!m_monthGrid) return;

    while (auto* item = m_monthGrid->takeAt(0)) {
        delete item;
    }

    if (m_monthCards.isEmpty()) {
        if (!m_monthEmpty) {
            m_monthEmpty = new QLabel(tr_no_months_available_9220b0(), m_monthSection);
            m_monthEmpty->setObjectName("emptyMsg");
            m_monthEmpty->setAlignment(Qt::AlignCenter);
        }
        m_monthGrid->addWidget(m_monthEmpty, 0, 0, 1, 3);
        return;
    }

    const int cols = 3;
    for (int i = 0; i < m_monthCards.size(); ++i) {
        auto* card = m_monthCards[i];
        if (!card) continue;
        card->setCardIndex(i);
        card->setMonthIndex(i);
        card->setFixedSize(370, 168);
        card->show();
        m_monthGrid->addWidget(card, i / cols, i % cols);
    }
}

void ResultsWidget::onSwapMonthCards(int fromIdx, int toIdx)
{
    if (fromIdx < 0 || fromIdx >= m_monthCards.size()) return;
    if (toIdx < 0 || toIdx >= m_monthCards.size()) return;
    m_monthCards.swapItemsAt(fromIdx, toIdx);
    m_monthOrder.swapItemsAt(fromIdx, toIdx);
    rebuildMonthGrid();
}

void ResultsWidget::addCard(const ChartRequest& request, QChartView* view)
{
    auto* card = new DraggableChartCard(request.title.isEmpty() ? metricDisplayName(request.metricA) : request.title, view, m_container);
    card->setCardIndex(m_nextCardId++);
    connect(card, &DraggableChartCard::swapRequested, this, &ResultsWidget::onSwapFlowItems);
    connect(card, &DraggableChartCard::insertSeparatorRequested, this, &ResultsWidget::onAddSeparatorAfter);
    connect(card, &DraggableChartCard::hideRequested, this, &ResultsWidget::onHideCard);
    connect(card, &DraggableChartCard::removeRequested, this, &ResultsWidget::onRemoveCard);
    connect(card, &DraggableChartCard::editRequested, this, [this](int) { emit editChartsRequested(); });
    m_cards.append(card);
    m_cardRequests.append(request);
}

void ResultsWidget::addHiddenCard(const ChartRequest& request, QChartView* view)
{
    auto* card = new DraggableChartCard(request.title.isEmpty() ? metricDisplayName(request.metricA) : request.title, view, m_container);
    card->setCardIndex(m_nextCardId++);
    connect(card, &DraggableChartCard::swapRequested, this, &ResultsWidget::onSwapFlowItems);
    connect(card, &DraggableChartCard::insertSeparatorRequested, this, &ResultsWidget::onAddSeparatorAfter);
    connect(card, &DraggableChartCard::removeRequested, this, &ResultsWidget::onRemoveCard);
    connect(card, &DraggableChartCard::editRequested, this, [this](int) { emit editChartsRequested(); });
    card->hide();
    m_hiddenCards.append(card);
    m_hiddenRequests.append(request);
}

static void normalizeSeparatorRuns(QVector<ResultFlowItem>& flow)
{
    QVector<ResultFlowItem> cleaned;
    cleaned.reserve(flow.size());

    for (const auto& item : flow) {
        if (item.kind == ResultFlowItemKind::PageSeparator) {
            if (cleaned.isEmpty())
                continue;
            if (cleaned.last().kind == ResultFlowItemKind::PageSeparator)
                continue;
        }
        cleaned.append(item);
    }

    while (!cleaned.isEmpty() && cleaned.last().kind == ResultFlowItemKind::PageSeparator)
        cleaned.removeLast();

    flow = cleaned;
}

void ResultsWidget::rebuildFlow()
{
    if (!m_flowLayout) return;

    normalizeSeparatorRuns(m_flowOrder);

    // Keep the saved flow clean so hidden month cards never reappear later.
    QList<ResultFlowItem> filteredFlow;
    QSet<int> seenMonths;
    for (const auto& item : m_flowOrder) {
        if (item.kind == ResultFlowItemKind::MonthCard) {
            if (item.index < 0 || item.index >= m_monthCards.size()) continue;
            if (!m_visibleMonths.contains(item.index)) continue;
            if (seenMonths.contains(item.index)) continue;
            seenMonths.insert(item.index);
        }
        filteredFlow.append(item);
    }
    if (filteredFlow != m_flowOrder)
        m_flowOrder = filteredFlow;

    while (auto* item = m_flowLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->hide();
            item->widget()->setParent(nullptr);
        }
        delete item;
    }

    if (m_flowOrder.isEmpty()) {
        if (!m_flowEmpty) {
            m_flowEmpty = new QLabel(tr_no_results_available_669e79(), m_flowSection);
            m_flowEmpty->setObjectName("emptyMsg");
            m_flowEmpty->setAlignment(Qt::AlignCenter);
        }
        m_flowLayout->addWidget(m_flowEmpty);
        return;
    }

    for (int i = 0; i < m_flowOrder.size(); ++i) {
        auto& item = m_flowOrder[i];
        if (item.kind == ResultFlowItemKind::MonthCard) {
            // item.index: 0 = "All months" summary card, 1-12 = individual month cards
            if (item.index < 0 || item.index >= m_monthCards.size()) continue;
            if (!m_visibleMonths.contains(item.index)) continue;
            auto* card = m_monthCards[item.index];
            if (!card) continue;
            card->setFlowIndex(i);
            card->setMinimumSize(item.index == 0 ? QSize(480, 168) : QSize(370, 168));
            card->setMaximumHeight(220);
            card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            card->show();
            m_flowLayout->addWidget(card);
        } else if (item.kind == ResultFlowItemKind::ChartCard) {
            DraggableChartCard* card = nullptr;
            for (auto* c : m_cards) {
                if (c && c->cardIndex() == item.index) { card = c; break; }
            }
            if (!card) continue;
            card->setFlowIndex(i);
            card->setMinimumSize(460, 360);
            card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            card->show();
            m_flowLayout->addWidget(card);
        } else {
            PageSeparatorCard* sep = m_separatorCards.value(item.id, nullptr);
            if (!sep) {
                sep = new PageSeparatorCard(m_flowSection);
                sep->setSeparatorId(item.id);
                connect(sep, &PageSeparatorCard::swapRequested, this, &ResultsWidget::onSwapFlowItems);
                connect(sep, &PageSeparatorCard::removeRequested, this, &ResultsWidget::onRemoveSeparator);
                m_separatorCards.insert(item.id, sep);
            }
            sep->setFlowIndex(i);
            sep->show();
            m_flowLayout->addWidget(sep);
        }
    }
}

void ResultsWidget::rebuildGrid()
{
    if (!m_grid) return;
    while (auto* item = m_grid->takeAt(0)) {
        delete item;
    }
    if (m_emptyState) {
        m_emptyState->deleteLater();
        m_emptyState = nullptr;
    }
    const int cols = 2;
    for (int i = 0; i < m_cards.size(); ++i) {
        m_cards[i]->setFixedSize(460, 360);
        m_cards[i]->show();
        m_grid->addWidget(m_cards[i], i / cols, i % cols);
    }
    if (m_cards.isEmpty()) {
        m_emptyState = new QLabel(tr_no_charts_selected_7a4c8f(), m_container);
        m_emptyState->setObjectName("emptyMsg");
        m_emptyState->setAlignment(Qt::AlignCenter);
        m_grid->addWidget(m_emptyState, 0, 0, 1, 2);
    }
}

void ResultsWidget::rebuildHiddenMenu()
{
    if (!m_hiddenMenu || !m_hiddenBtn) return;
    m_hiddenMenu->clear();
    for (int i = 0; i < m_hiddenCards.size(); ++i) {
        QAction* act = m_hiddenMenu->addAction(m_hiddenCards[i]->title());
        act->setData(i);
    }
    m_hiddenBtn->setEnabled(!m_hiddenCards.isEmpty());
}

void ResultsWidget::onSwapCards(int fromIdx, int toIdx)
{
    if (fromIdx < 0 || fromIdx >= m_cards.size()) return;
    if (toIdx < 0 || toIdx >= m_cards.size()) return;
    m_cards.swapItemsAt(fromIdx, toIdx);
    m_cardRequests.swapItemsAt(fromIdx, toIdx);
    rebuildGrid();
    emit resultsStateChanged();
}

void ResultsWidget::onHideCard(int cardIndex)
{
    int pos = -1;
    for (int i = 0; i < m_cards.size(); ++i) {
        if (m_cards[i] && m_cards[i]->cardIndex() == cardIndex) { pos = i; break; }
    }
    if (pos < 0 || pos >= m_cards.size()) return;
    DraggableChartCard* card = m_cards.takeAt(pos);
    ChartRequest req = m_cardRequests.takeAt(pos);
    m_hiddenCards.append(card);
    m_hiddenRequests.append(req);
    for (int i = 0; i < m_flowOrder.size(); ++i) {
        if (m_flowOrder[i].kind == ResultFlowItemKind::ChartCard && m_flowOrder[i].index == cardIndex) {
            m_flowOrder.removeAt(i);
            break;
        }
    }
    card->hide();
    rebuildGrid();
    rebuildFlow();
    rebuildHiddenMenu();
    emit resultsStateChanged();
}

void ResultsWidget::onRestoreHidden(QAction* action)
{
    if (!action) return;
    const int idx = action->data().toInt();
    if (idx < 0 || idx >= m_hiddenCards.size()) return;
    DraggableChartCard* card = m_hiddenCards.takeAt(idx);
    ChartRequest req = m_hiddenRequests.takeAt(idx);
    m_cards.append(card);
    m_cardRequests.append(req);
    card->show();
    rebuildGrid();
    ResultFlowItem item;
    item.kind = ResultFlowItemKind::ChartCard;
    item.index = card->cardIndex();
    m_flowOrder.append(item);
    rebuildFlow();
    rebuildHiddenMenu();
    emit resultsStateChanged();
}

void ResultsWidget::onRemoveCard(int cardIndex)
{
    int pos = -1;
    for (int i = 0; i < m_cards.size(); ++i) {
        if (m_cards[i] && m_cards[i]->cardIndex() == cardIndex) { pos = i; break; }
    }
    if (pos >= 0 && pos < m_cards.size()) {
        auto* card = m_cards.takeAt(pos);
        m_cardRequests.removeAt(pos);
        for (int i = 0; i < m_flowOrder.size(); ++i) {
            if (m_flowOrder[i].kind == ResultFlowItemKind::ChartCard && m_flowOrder[i].index == cardIndex) {
                m_flowOrder.removeAt(i);
                break;
            }
        }
        card->deleteLater();
        rebuildFlow();
        rebuildGrid();
        rebuildHiddenMenu();
        emit resultsStateChanged();
        return;
    }

    for (int i = 0; i < m_hiddenCards.size(); ++i) {
        if (!m_hiddenCards[i] || m_hiddenCards[i]->cardIndex() != cardIndex) continue;
        auto* card = m_hiddenCards.takeAt(i);
        m_hiddenRequests.removeAt(i);
        card->deleteLater();
        rebuildHiddenMenu();
        emit resultsStateChanged();
        return;
    }
}

void ResultsWidget::onSwapFlowItems(int fromIdx, int toIdx)
{
    if (fromIdx < 0 || fromIdx >= m_flowOrder.size()) return;
    if (toIdx < 0 || toIdx >= m_flowOrder.size()) return;
    m_flowOrder.swapItemsAt(fromIdx, toIdx);
    rebuildFlow();
    emit resultsStateChanged();
}

void ResultsWidget::onAddSeparatorAfter(int flowIndex)
{
    if (flowIndex < 0 || flowIndex >= m_flowOrder.size()) return;
    if (m_flowOrder[flowIndex].kind == ResultFlowItemKind::PageSeparator) return;

    const int insertPos = flowIndex + 1;
    if (insertPos >= m_flowOrder.size()) return;
    if (m_flowOrder[insertPos].kind == ResultFlowItemKind::PageSeparator) return;

    ResultFlowItem sep;
    sep.kind = ResultFlowItemKind::PageSeparator;
    sep.id = ++m_nextSeparatorId;
    m_flowOrder.insert(insertPos, sep);
    rebuildFlow();
    emit resultsStateChanged();
}

void ResultsWidget::onRemoveSeparator(int separatorId)
{
    for (int i = 0; i < m_flowOrder.size(); ++i) {
        if (m_flowOrder[i].kind == ResultFlowItemKind::PageSeparator && m_flowOrder[i].id == separatorId) {
            m_flowOrder.removeAt(i);
            break;
        }
    }
    if (m_separatorCards.contains(separatorId)) {
        auto* w = m_separatorCards.take(separatorId);
        if (w) w->deleteLater();
    }
    rebuildFlow();
    emit resultsStateChanged();
}

QList<QChartView*> ResultsWidget::allChartViews() const
{
    QList<QChartView*> list;
    for (auto* c : m_cards) list << c->chartView();
    return list;
}

QList<ChartRequest> ResultsWidget::chartRequests() const
{
    return m_cardRequests;
}

QList<ChartRequest> ResultsWidget::hiddenChartRequests() const
{
    return m_hiddenRequests;
}

QImage ResultsWidget::renderFlowItemImage(const ResultFlowItem& item) const
{
    QWidget* widget = nullptr;

    if (item.kind == ResultFlowItemKind::MonthCard) {
        if (item.index >= 0 && item.index < m_monthCards.size())
            widget = m_monthCards[item.index];
    } else if (item.kind == ResultFlowItemKind::ChartCard) {
        for (auto* card : m_cards) {
            if (card && card->cardIndex() == item.index) {
                widget = card;
                break;
            }
        }
    }

    if (!widget)
        return {};

    const QSize sz = widget->size().isValid() ? widget->size() : widget->sizeHint();
    if (sz.isEmpty())
        return {};

    widget->ensurePolished();

    QImage img(sz, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    widget->render(&img);
    return img;
}


static void applyThemeToChart(QChart* chart)
{
    if (g_lightMode) {
        chart->setBackgroundBrush(QBrush(QColor("#ffffff")));
        chart->setPlotAreaBackgroundBrush(QBrush(QColor("#f8f9ff")));
        chart->setTitleBrush(QBrush(QColor("#1e2340")));
        chart->legend()->setLabelColor(QColor("#5a6490"));
    } else {
        chart->setBackgroundBrush(QBrush(QColor("#151929")));
        chart->setPlotAreaBackgroundBrush(QBrush(QColor("#0f1320")));
        chart->setTitleBrush(QBrush(QColor("#c8d0ed")));
        chart->legend()->setLabelColor(QColor("#8892b8"));
    }
    chart->setPlotAreaBackgroundVisible(true);
    chart->setTitleFont(QFont("Segoe UI", 12, QFont::Bold));
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setFont(QFont("Segoe UI", 9));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setMargins(QMargins(4, 4, 4, 4));
}

void ResultsWidget::applyChartTheme(QChart* chart, const QString& title)
{
    if (!chart) return;
    applyThemeToChart(chart);
    chart->setTitle(title);
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

QChartView* ResultsWidget::makePieChart(const QString& title,
                                        const QStringList& labels,
                                        const QList<double>& values)
{
    auto* series = new QPieSeries;
    const QList<double> normalized = normalizePercentValues(values);
    const QList<double> display = displayPercentValues(normalized);

    QColor borderCol = g_lightMode ? QColor("#f4f6fb") : QColor("#0d1020");
    QStringList legendLabels;
    QStringList legendColors;
    for (int i = 0; i < labels.size() && i < normalized.size(); ++i) {
        const double v = normalized[i];
        if (v < 0.001)
            continue;
        const QColor c = kPal[i % kPal.size()];
        auto* sl = series->append(labels[i], v);
        sl->setColor(c);
        sl->setBorderColor(borderCol);
        sl->setLabelVisible(true);
        sl->setLabelPosition(QPieSlice::LabelOutside);
        sl->setLabelArmLengthFactor(0.26);
        sl->setLabelColor(g_lightMode ? QColor("#1e2340") : QColor("#ffffff"));
        sl->setLabel(QString::number(display.value(i), 'f', 1) + QStringLiteral("%"));
        legendLabels << (labels[i] + QStringLiteral(": ") + QString::number(display.value(i), 'f', 1) + QStringLiteral("%"));
        legendColors << c.name();
        QObject::connect(sl, &QPieSlice::hovered, sl, [sl](bool on) {
            sl->setExploded(on);
        });
    }

    series->setPieSize(0.66);
    auto* chart = new QChart;
    chart->addSeries(series);
    chart->legend()->setVisible(true);
    chart->setMargins(QMargins(18, 18, 18, 18));
    applyThemeToChart(chart);
    chart->setTitle(title);

    auto* view = makeChartView(chart, false);
    view->setProperty("legendLabels", legendLabels);
    view->setProperty("legendColors", legendColors);
    return view;
}

QChartView* ResultsWidget::makeCandleChart(const QString& title,
                                           const QStringList& labels,
                                           const QList<double>& values)
{
    auto* series = new QCandlestickSeries;
    series->setIncreasingColor(QColor("#3ecf8e"));
    series->setDecreasingColor(QColor("#e05c6a"));
    series->setBodyOutlineVisible(false);

    for (int i = 0; i < values.size(); ++i) {
        const double cur = values[i];
        const double open = 0.0;
        const double close = cur;
        double hi = qMax(open, close) * 1.02;
        double lo = qMin(open, close) * 0.98;
        if (hi <= lo + 0.001) hi = lo + 1;
        auto* cs = new QCandlestickSet(open, hi, lo, close, static_cast<qreal>(i));
        series->append(cs);
    }

    auto* chart = new QChart;
    chart->addSeries(series);
    applyThemeToChart(chart);
    chart->setTitle(title);

    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    series->attachAxis(axX);
    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    axY->setLabelFormat("$%'i");
    double maxV = 0.0;
    for (double v : values) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    axY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axY);

    auto* legInc = new QLineSeries;
    legInc->setName(tr_increasing_c5cd67());
    legInc->setColor(QColor("#3ecf8e"));
    auto* legDec = new QLineSeries;
    legDec->setName(tr_decreasing_b4c279());
    legDec->setColor(QColor("#e05c6a"));
    chart->addSeries(legInc);
    chart->addSeries(legDec);
    legInc->attachAxis(axX);
    legInc->attachAxis(axY);
    legDec->attachAxis(axX);
    legDec->attachAxis(axY);

    chart->legend()->setVisible(false);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("legendLabels", QStringList{tr_increasing_c5cd67() + QStringLiteral(" (100.0%)"), tr_decreasing_b4c279() + QStringLiteral(" (100.0%)")});
    view->setProperty("legendColors", QStringList{QString("#3ecf8e"), QString("#e05c6a")});
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : values) vl << x;
    view->setProperty("chartValues", vl);
    QVariantList vp; for (double x : computePercentages(values)) vp << x;
    view->setProperty("chartPercents", vp);
    view->setProperty("chartType", "candle");
    view->setProperty("chartTitle", title);
    return view;
}

QChartView* ResultsWidget::makeRankedBarChart(const QString& title,
                                              const QStringList& labels,
                                              const QList<double>& values)
{
    auto* set = new QBarSet(title);
    set->setColor(QColor("#4f86f7"));
    set->setBorderColor(Qt::transparent);
    for (double v : values) *set << v;

    auto* series = new QBarSeries;
    series->append(set);
    series->setBarWidth(0.7);

    auto* chart = new QChart;
    chart->addSeries(series);
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    series->attachAxis(axX);
    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : values) vl << x;
    view->setProperty("chartValues", vl);
    QVariantList vp; for (double x : computePercentages(values)) vp << x;
    view->setProperty("chartPercents", vp);
    set->setLabel(title + QStringLiteral(" (") + percentText(100.0) + QStringLiteral(")"));
    view->setProperty("chartType", "rankedbar");
    view->setProperty("chartTitle", title);
    return view;
}

QChartView* ResultsWidget::makeSingleLineChart(const QString& title,
                                               const QStringList& labels,
                                               const QList<double>& values)
{
    auto* line = new QLineSeries;
    line->setName(title);
    line->setColor(QColor("#4f86f7"));
    for (int i = 0; i < values.size(); ++i)
        line->append(i + 0.5, values[i]);

    auto* chart = new QChart;
    chart->addSeries(line);
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(false);

    auto* axisX = new QCategoryAxis;
    for (int i = 0; i < labels.size(); ++i)
        axisX->append(labels[i], i + 1);
    axisX->setRange(0, qMax(1, labels.size()));
    axisX->setLabelsFont(QFont("Segoe UI", 8));
    axisX->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisX->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    chart->addAxis(axisX, Qt::AlignBottom);
    line->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setLabelsFont(QFont("Segoe UI", 8));
    axisY->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisY->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    double maxV = 0.0;
    for (double v : values) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    axisY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axisY, Qt::AlignLeft);
    line->attachAxis(axisY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : values) vl << x;
    view->setProperty("chartValues", vl);
    QVariantList vp; for (double x : computePercentages(values)) vp << x;
    view->setProperty("chartPercents", vp);
    line->setName(title + QStringLiteral(" (") + percentText(100.0) + QStringLiteral(")"));
    view->setProperty("chartType", "line");
    view->setProperty("chartTitle", title);
    return view;
}

QChartView* ResultsWidget::makeCompareCandleChart(const QString& title,
                                                  const QStringList& labels,
                                                  const QList<double>& seriesA,
                                                  const QList<double>& seriesB,
                                                  const QString& nameA,
                                                  const QString& nameB)
{
    auto* candA = new QCandlestickSeries;
    auto* candB = new QCandlestickSeries;
    candA->setName(nameA);
    candB->setName(nameB);
    candA->setIncreasingColor(QColor("#3ecf8e"));
    candA->setDecreasingColor(QColor("#e05c6a"));
    candB->setIncreasingColor(QColor("#f0a500"));
    candB->setDecreasingColor(QColor("#7cc4ff"));
    candA->setBodyOutlineVisible(false);
    candB->setBodyOutlineVisible(false);

    const int n = qMax(seriesA.size(), seriesB.size());
    for (int i = 0; i < n; ++i) {
        const double a = i < seriesA.size() ? seriesA[i] : 0.0;
        const double b = i < seriesB.size() ? seriesB[i] : 0.0;
        double hiA = qMax(0.0, a) * 1.02;
        double loA = qMin(0.0, a) * 0.98;
        if (hiA <= loA + 0.001) hiA = loA + 1;
        double hiB = qMax(0.0, b) * 1.02;
        double loB = qMin(0.0, b) * 0.98;
        if (hiB <= loB + 0.001) hiB = loB + 1;
        candA->append(new QCandlestickSet(0.0, hiA, loA, a, static_cast<qreal>(i) - 0.18));
        candB->append(new QCandlestickSet(0.0, hiB, loB, b, static_cast<qreal>(i) + 0.18));
    }

    auto* chart = new QChart;
    chart->addSeries(candA);
    chart->addSeries(candB);
    applyChartTheme(chart, title);
    chart->legend()->setVisible(false);

    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    candA->attachAxis(axX);
    candB->attachAxis(axX);

    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    double maxV = 0.0;
    for (double v : seriesA) maxV = qMax(maxV, qAbs(v));
    for (double v : seriesB) maxV = qMax(maxV, qAbs(v));
    if (maxV < 0.001) maxV = 1.0;
    axY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axY, Qt::AlignLeft);
    candA->attachAxis(axY);
    candB->attachAxis(axY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("legendLabels", QStringList{nameA, nameB});
    view->setProperty("legendColors", QStringList{QString("#3ecf8e"), QString("#f0a500")});
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : seriesA) vl << x;
    view->setProperty("chartValues", vl);
    view->setProperty("chartType", "comparecandle");
    view->setProperty("chartTitle", title);
    view->setProperty("chartLabels2", labels);
    QVariantList vl2; for (double x : seriesB) vl2 << x;
    view->setProperty("chartValues2", vl2);
    QVariantList vpm; QVariantList pA; for (double x : computePercentages(seriesA)) pA << x; QVariantList pB; for (double x : computePercentages(seriesB)) pB << x; vpm << pA << pB;
    view->setProperty("chartValuesMulti", QVariantList{vl, vl2});
    view->setProperty("chartPercentsMulti", vpm);
    view->setProperty("chartSeriesA", nameA);
    view->setProperty("chartSeriesB", nameB);
    return view;
}

QChartView* ResultsWidget::makeCompareBarChart(const QString& title,
                                               const QStringList& labels,
                                               const QList<double>& seriesA,
                                               const QList<double>& seriesB,
                                               const QString& nameA,
                                               const QString& nameB)
{
    auto* setA = new QBarSet(nameA);
    auto* setB = new QBarSet(nameB);
    setA->setColor(QColor("#f0a500"));
    setB->setColor(QColor("#3ecf8e"));
    setA->setBorderColor(Qt::transparent);
    setB->setBorderColor(Qt::transparent);
    for (double v : seriesA) *setA << v;
    for (double v : seriesB) *setB << v;

    auto* series = new QBarSeries;
    series->append(setA);
    series->append(setB);
    series->setBarWidth(0.65);

    auto* chart = new QChart;
    chart->addSeries(series);
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(true);

    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    series->attachAxis(axX);
    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : seriesA) vl << x;
    view->setProperty("chartValues", vl);
    view->setProperty("chartType", "barcompare");
    view->setProperty("chartTitle", title);
    view->setProperty("chartLabels2", labels);
    QVariantList vl2; for (double x : seriesB) vl2 << x;
    view->setProperty("chartValues2", vl2);
    QVariantList vpm; QVariantList pA; for (double x : computePercentages(seriesA)) pA << x; QVariantList pB; for (double x : computePercentages(seriesB)) pB << x; vpm << pA << pB;
    view->setProperty("chartValuesMulti", QVariantList{vl, vl2});
    view->setProperty("chartPercentsMulti", vpm);
    view->setProperty("chartSeriesA", nameA);
    view->setProperty("chartSeriesB", nameB);
    return view;
}

QChartView* ResultsWidget::makeCompareLineChart(const QString& title,
                                                const QStringList& labels,
                                                const QList<double>& seriesA,
                                                const QList<double>& seriesB,
                                                const QString& nameA,
                                                const QString& nameB)
{
    auto* lineA = new QLineSeries;
    auto* lineB = new QLineSeries;
    lineA->setName(nameA);
    lineB->setName(nameB);
    lineA->setColor(QColor("#f0a500"));
    lineB->setColor(QColor("#3ecf8e"));

    const int n = qMin(seriesA.size(), seriesB.size());
    for (int i = 0; i < n; ++i) {
        lineA->append(i + 0.5, seriesA[i]);
        lineB->append(i + 0.5, seriesB[i]);
    }

    auto* chart = new QChart;
    chart->addSeries(lineA);
    chart->addSeries(lineB);
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(true);

    auto* axisX = new QCategoryAxis;
    for (int i = 0; i < labels.size(); ++i)
        axisX->append(labels[i], i + 1);
    axisX->setRange(0, qMax(1, labels.size()));
    axisX->setLabelsFont(QFont("Segoe UI", 8));
    axisX->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisX->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    chart->addAxis(axisX, Qt::AlignBottom);
    lineA->attachAxis(axisX);
    lineB->attachAxis(axisX);

    auto* axisY = new QValueAxis;
    axisY->setLabelsFont(QFont("Segoe UI", 8));
    axisY->setLabelsColor(g_lightMode ? QColor("#5a6490") : QColor("#8892b8"));
    axisY->setGridLineColor(g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445"));
    chart->addAxis(axisY, Qt::AlignLeft);
    lineA->attachAxis(axisY);
    lineB->attachAxis(axisY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode
        ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
        : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    QVariantList vl; for (double x : seriesA) vl << x;
    view->setProperty("chartValues", vl);
    view->setProperty("chartType", "linecompare");
    view->setProperty("chartTitle", title);
    view->setProperty("chartLabels2", labels);
    QVariantList vl2; for (double x : seriesB) vl2 << x;
    view->setProperty("chartValues2", vl2);
    QVariantList vpm; QVariantList pA; for (double x : computePercentages(seriesA)) pA << x; QVariantList pB; for (double x : computePercentages(seriesB)) pB << x; vpm << pA << pB;
    view->setProperty("chartValuesMulti", QVariantList{vl, vl2});
    view->setProperty("chartPercentsMulti", vpm);
    view->setProperty("chartSeriesA", nameA);
    view->setProperty("chartSeriesB", nameB);
    return view;
}

QChartView* ResultsWidget::makeMultiCompareBarChart(const QString& title,
                                                   const QStringList& labels,
                                                   const QList<QList<double>>& seriesList,
                                                   const QStringList& names)
{
    auto* series = new QBarSeries;
    double maxV = 0.0;
    for (int i = 0; i < seriesList.size(); ++i) {
        const auto& vals = seriesList[i];
        auto* set = new QBarSet(names.value(i, QStringLiteral("Series %1").arg(i + 1)));
        const QColor col = kPal[i % kPal.size()];
        set->setColor(col);
        set->setBorderColor(Qt::transparent);
        for (double v : vals) {
            *set << v;
            maxV = qMax(maxV, qAbs(v));
        }
        series->append(set);
    }
    series->setBarWidth(0.72);

    auto* chart = new QChart;
    chart->addSeries(series);
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(true);

    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    series->attachAxis(axX);
    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    if (maxV < 0.001) maxV = 1.0;
    axY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
                                    : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    view->setProperty("chartType", "barcompare");
    view->setProperty("chartTitle", title);
    view->setProperty("chartSeriesNames", names);
    QVariantList valuesProp;
    QVariantList percentProp;
    for (const auto& vals : seriesList) {
        QVariantList one; for (double x : vals) one << x;
        valuesProp << one;
        QVariantList pp; for (double x : computePercentages(vals)) pp << x;
        percentProp << pp;
    }
    view->setProperty("chartValuesMulti", valuesProp);
    view->setProperty("chartPercentsMulti", percentProp);
    return view;
}

QChartView* ResultsWidget::makeMultiCompareLineChart(const QString& title,
                                                    const QStringList& labels,
                                                    const QList<QList<double>>& seriesList,
                                                    const QStringList& names)
{
    auto* chart = new QChart;
    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    double maxV = 0.0;
    for (int s = 0; s < seriesList.size(); ++s) {
        const auto& vals = seriesList[s];
        auto* line = new QLineSeries;
        line->setName(names.value(s, QStringLiteral("Series %1").arg(s + 1)));
        line->setColor(kPal[s % kPal.size()]);
        const int n = vals.size();
        for (int i = 0; i < n; ++i) {
            line->append(i + 0.5, vals[i]);
            maxV = qMax(maxV, qAbs(vals[i]));
        }
        chart->addSeries(line);
    }
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(true);

    auto* axisX = new QCategoryAxis;
    for (int i = 0; i < labels.size(); ++i)
        axisX->append(labels[i], i + 1);
    axisX->setRange(0, qMax(1, labels.size()));
    axisX->setLabelsFont(QFont("Segoe UI", 8));
    axisX->setLabelsColor(axisCol);
    axisX->setGridLineColor(gridCol);
    chart->addAxis(axisX, Qt::AlignBottom);

    auto* axisY = new QValueAxis;
    axisY->setLabelsFont(QFont("Segoe UI", 8));
    axisY->setLabelsColor(axisCol);
    axisY->setGridLineColor(gridCol);
    if (maxV < 0.001) maxV = 1.0;
    axisY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axisY, Qt::AlignLeft);

    for (auto* s : chart->series()) {
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
                                    : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    view->setProperty("chartType", "linecompare");
    view->setProperty("chartTitle", title);
    view->setProperty("chartSeriesNames", names);
    QVariantList valuesProp;
    QVariantList percentProp;
    for (const auto& vals : seriesList) {
        QVariantList one; for (double x : vals) one << x;
        valuesProp << one;
        QVariantList pp; for (double x : computePercentages(vals)) pp << x;
        percentProp << pp;
    }
    view->setProperty("chartValuesMulti", valuesProp);
    view->setProperty("chartPercentsMulti", percentProp);
    return view;
}

QChartView* ResultsWidget::makeMultiComparePieChart(const QString& title,
                                                    const QStringList& names,
                                                    const QList<double>& values,
                                                    double referenceValue)
{
    Q_UNUSED(referenceValue);
    auto* series = new QPieSeries;
    const QList<double> normalized = normalizePercentValues(values);
    const QList<double> display = displayPercentValues(normalized);
    QStringList legendLabels;
    QStringList legendColors;
    for (int i = 0; i < normalized.size(); ++i) {
        const double sliceValue = qAbs(normalized.value(i));
        if (sliceValue < 0.001)
            continue;
        const QColor color = (names.value(i) == tr_remaining_1f3b2a()) ? QColor("#8f97b4") : kPal[i % kPal.size()];
        auto* sl = series->append(names.value(i, QStringLiteral("Series %1").arg(i + 1)), sliceValue);
        sl->setColor(color);
        sl->setLabelVisible(true);
        sl->setLabelPosition(QPieSlice::LabelOutside);
        sl->setLabelArmLengthFactor(0.28);
        sl->setLabelColor(g_lightMode ? QColor("#1e2340") : QColor("#ffffff"));
        sl->setLabel(QString::number(display.value(i), 'f', 1) + QStringLiteral("%"));
        legendLabels << (names.value(i, QStringLiteral("Series %1").arg(i + 1)) + QStringLiteral(": ") + QString::number(display.value(i), 'f', 1) + QStringLiteral("%"));
        legendColors << color.name();
    }
    series->setPieSize(0.64);
    auto* chart = new QChart;
    chart->addSeries(series);
    chart->setMargins(QMargins(18, 18, 18, 18));
    applyChartTheme(chart, title);
    chart->legend()->setVisible(false);
    auto* view = makeChartView(chart, false);
    view->setProperty("legendLabels", legendLabels);
    view->setProperty("legendColors", legendColors);
    view->setProperty("chartType", "comparepie");
    view->setProperty("chartTitle", title);
    return view;
}

QChartView* ResultsWidget::makeMultiCompareCandleChart(const QString& title,
                                                      const QStringList& labels,
                                                      const QList<QList<double>>& seriesList,
                                                      const QStringList& names)
{
    auto* chart = new QChart;
    QColor axisCol = g_lightMode ? QColor("#5a6490") : QColor("#8892b8");
    QColor gridCol = g_lightMode ? QColor("#e5e7eb") : QColor("#1e2445");
    double maxV = 0.0;
    for (int s = 0; s < seriesList.size(); ++s) {
        const auto& vals = seriesList[s];
        auto* series = new QCandlestickSeries;
        series->setName(names.value(s, QStringLiteral("Series %1").arg(s + 1)));
        const QColor col = kPal[s % kPal.size()];
        series->setIncreasingColor(col);
        series->setDecreasingColor(col.darker(120));
        series->setBodyOutlineVisible(false);
        const double offset = (s - (seriesList.size() - 1) / 2.0) * 0.18;
        for (int i = 0; i < vals.size(); ++i) {
            const double a = vals[i];
            const double hi = qMax(0.0, a) * 1.02 + 0.01;
            const double lo = qMin(0.0, a) * 0.98;
            maxV = qMax(maxV, qAbs(a));
            series->append(new QCandlestickSet(0.0, hi, lo, a, static_cast<qreal>(i) + offset));
        }
        chart->addSeries(series);
    }
    applyThemeToChart(chart);
    chart->setTitle(title);
    chart->legend()->setVisible(true);

    auto* axX = new QBarCategoryAxis;
    axX->append(labels);
    axX->setLabelsColor(axisCol);
    axX->setLabelsAngle(-45);
    axX->setGridLineColor(gridCol);
    axX->setLabelsFont(QFont("Segoe UI", 8));
    chart->addAxis(axX, Qt::AlignBottom);
    for (auto* s : chart->series()) s->attachAxis(axX);

    auto* axY = new QValueAxis;
    axY->setLabelsColor(axisCol);
    axY->setGridLineColor(gridCol);
    axY->setLabelsFont(QFont("Segoe UI", 8));
    if (maxV < 0.001) maxV = 1.0;
    axY->setRange(0.0, maxV * 1.1);
    chart->addAxis(axY, Qt::AlignLeft);
    for (auto* s : chart->series()) s->attachAxis(axY);

    auto* view = makeChartView(chart);
    view->setStyleSheet(g_lightMode ? "background:#ffffff; border:none; border-radius:0 0 10px 10px;"
                                    : "background:#151929; border:none; border-radius:0 0 10px 10px;");
    view->setProperty("chartLabels", labels);
    view->setProperty("chartType", "comparecandle");
    view->setProperty("chartTitle", title);
    view->setProperty("chartSeriesNames", names);
    QVariantList valuesProp;
    QVariantList percentProp;
    for (const auto& vals : seriesList) {
        QVariantList one; for (double x : vals) one << x;
        valuesProp << one;
        QVariantList pp; for (double x : computePercentages(vals)) pp << x;
        percentProp << pp;
    }
    view->setProperty("chartValuesMulti", valuesProp);
    view->setProperty("chartPercentsMulti", percentProp);
    return view;
}

QChartView* ResultsWidget::makeComparePieChart(const QString& title,
                                               const QString& nameA,
                                               const QString& nameB,
                                               double valueA,
                                               double valueB,
                                               double referenceValue)
{
    auto* series = new QPieSeries;
    if (valueA == 0 && valueB == 0) {
        valueA = 1;
        valueB = 1;
    }
    const double total = qMax(0.0001, qAbs(valueA) + qAbs(valueB));
    const double reference = referenceValue > 0.0001 ? referenceValue : total;
    auto* a = series->append(nameA, qMax(0.0, valueA));
    auto* b = series->append(nameB, qMax(0.0, valueB));
    const QList<double> display = normalizePercentValues(QList<double>{qAbs(valueA), qAbs(valueB)});
    a->setLabelVisible(true);
    b->setLabelVisible(true);
    a->setLabelPosition(QPieSlice::LabelOutside);
    b->setLabelPosition(QPieSlice::LabelOutside);
    a->setLabelArmLengthFactor(0.18);
    b->setLabelArmLengthFactor(0.18);
    a->setLabelColor(g_lightMode ? QColor("#1e2340") : QColor("#ffffff"));
    b->setLabelColor(g_lightMode ? QColor("#1e2340") : QColor("#ffffff"));
    a->setLabel(QString::number(display.value(0), 'f', 1) + QStringLiteral("%"));
    b->setLabel(QString::number(display.value(1), 'f', 1) + QStringLiteral("%"));
    const QColor colA = kPal[0];
    const QColor colB = kPal[1];
    a->setColor(colA);
    b->setColor(colB);

    series->setPieSize(0.72);
    auto* chart = new QChart;
    chart->addSeries(series);
    applyChartTheme(chart, title);
    chart->legend()->setVisible(false);
    auto* view = makeChartView(chart, false);
    view->setProperty("legendLabels", QStringList{nameA, nameB});
    view->setProperty("legendColors", QStringList{colA.name(), colB.name()});
    return view;
}

static void appendSummaryLabel(QStringList& labels)
{
    labels << T("Summary", "الملخص");
}

static void appendSummaryValue(QList<double>& values)
{
    if (values.isEmpty())
        return;
    double total = 0.0;
    for (double v : values) total += v;
    values << total;
}

QChartView* ResultsWidget::createChartView(const AppData& data, const ChartRequest& request)
{
    QStringList labels;
    const QList<int>* months = request.months.isEmpty() ? nullptr : &request.months;
    QList<double> a = metricSeriesValues(data, request.metricA, &labels, months, request.accountFilter);
    QList<double> b;
    QString title = request.title.isEmpty() ? metricDisplayName(request.metricA) : request.title;
    const bool includeSummaryPoint = request.includeSummaryPoint && request.kind != ChartKind::Pie && request.kind != ChartKind::ComparePie;
    const bool usePercentBase = request.comparePieBaseMetric >= M_SALES && request.comparePieBaseMetric < M_COUNT;

    QList<MetricId> compareMetrics = request.compareMetrics;
    if (compareMetrics.size() < 2 && (request.kind == ChartKind::CompareBar || request.kind == ChartKind::CompareLine || request.kind == ChartKind::ComparePie || request.kind == ChartKind::Candle)) {
        compareMetrics.clear();
        compareMetrics << request.metricA << request.metricB;
    }

    if (compareMetrics.size() > 2 && (request.kind == ChartKind::CompareBar || request.kind == ChartKind::CompareLine || request.kind == ChartKind::ComparePie || request.kind == ChartKind::Candle)) {
        QList<QList<double>> seriesList;
        QStringList names;
        for (MetricId id : compareMetrics) {
            QStringList labelsForMetric;
            QList<double> values = metricSeriesValues(data, id, &labelsForMetric, months, request.accountFilter);
            if (labels.isEmpty())
                labels = labelsForMetric;
            seriesList << values;
            names << metricDisplayName(id);
        }
        if (includeSummaryPoint) {
            appendSummaryLabel(labels);
            for (auto& values : seriesList)
                appendSummaryValue(values);
        }
        if (!usePercentBase && request.kind != ChartKind::ComparePie) {
            double grandTotal = 0.0;
            for (const auto& values : seriesList) grandTotal += totalAbsValue(values);
            if (grandTotal > 0.000001) {
                for (int i = 0; i < seriesList.size(); ++i)
                    names[i] += QStringLiteral(" (") + percentText((totalAbsValue(seriesList[i]) / grandTotal) * 100.0) + QStringLiteral(")");
            }
        }
        if (usePercentBase && request.kind != ChartKind::ComparePie) {
            const int baseIdx = compareMetrics.indexOf(request.comparePieBaseMetric);
            if (baseIdx >= 0 && baseIdx < seriesList.size()) {
                const QList<double> baseSeries = seriesList[baseIdx];
                for (int i = 0; i < seriesList.size(); ++i) {
                    names[i] += QStringLiteral(" (") + percentText(computeTotalPercentageAgainstBase(seriesList[i], baseSeries)) + QStringLiteral(")");
                    seriesList[i] = computePercentagesAgainstBase(seriesList[i], baseSeries);
                }
                title += QStringLiteral(" — ") + metricDisplayName(request.comparePieBaseMetric) + QStringLiteral(" = 100%");
            }
        }
        if (request.kind == ChartKind::CompareLine)
            return makeMultiCompareLineChart(title, labels, seriesList, names);
        if (request.kind == ChartKind::ComparePie) {
            QList<double> totals;
            for (const auto& values : seriesList) {
                double total = 0.0;
                for (double v : values) total += qAbs(v);
                totals << total;
            }
            QStringList pieLabels;
            QList<double> pieValues;
            buildComparePieSlices(names, totals, request.comparePieBaseMetric == M_COUNT ? -1 : compareMetrics.indexOf(request.comparePieBaseMetric), pieLabels, pieValues);
            return makeMultiComparePieChart(title, pieLabels, pieValues, 100.0);
        }
        if (request.kind == ChartKind::Candle)
            return makeMultiCompareCandleChart(title, labels, seriesList, names);
        return makeMultiCompareBarChart(title, labels, seriesList, names);
    }

    switch (request.kind) {
    case ChartKind::Pie:
        return makePieChart(title, labels, a);
    case ChartKind::Candle:
        if (!request.seriesB.isEmpty()) {
            QStringList labelsB;
            b = metricSeriesValues(data, request.metricB, &labelsB, months, request.accountFilter);
            if (includeSummaryPoint) {
                appendSummaryLabel(labels);
                appendSummaryValue(a);
                appendSummaryValue(b);
            }
            QString nameA = request.seriesA.isEmpty() ? metricDisplayName(request.metricA) : request.seriesA;
            QString nameB = request.seriesB.isEmpty() ? metricDisplayName(request.metricB) : request.seriesB;
            if (usePercentBase) {
                QList<double> baseSeries;
                if (request.comparePieBaseMetric == request.metricA) baseSeries = a;
                else if (request.comparePieBaseMetric == request.metricB) baseSeries = b;
                if (!baseSeries.isEmpty()) {
                    nameA += QStringLiteral(" (") + percentText(computeTotalPercentageAgainstBase(a, baseSeries)) + QStringLiteral(")");
                    nameB += QStringLiteral(" (") + percentText(computeTotalPercentageAgainstBase(b, baseSeries)) + QStringLiteral(")");
                    a = computePercentagesAgainstBase(a, baseSeries);
                    b = computePercentagesAgainstBase(b, baseSeries);
                    title += QStringLiteral(" — ") + metricDisplayName(request.comparePieBaseMetric) + QStringLiteral(" = 100%");
                }
            } else {
                const double totalAB = totalAbsValue(a) + totalAbsValue(b);
                if (totalAB > 0.000001) {
                    nameA += QStringLiteral(" (") + percentText((totalAbsValue(a) / totalAB) * 100.0) + QStringLiteral(")");
                    nameB += QStringLiteral(" (") + percentText((totalAbsValue(b) / totalAB) * 100.0) + QStringLiteral(")");
                }
            }
            return makeCompareCandleChart(title, labels, a, b, nameA, nameB);
        }
        if (includeSummaryPoint) {
            appendSummaryLabel(labels);
            appendSummaryValue(a);
        }
        if (request.metricA == M_EXPENSES)
            return makeRankedBarChart(title, labels, a);
        return makeCandleChart(title, labels, a);
    case ChartKind::RankedBar:
    case ChartKind::MetricBar:
        if (includeSummaryPoint) {
            appendSummaryLabel(labels);
            appendSummaryValue(a);
        }
        return makeRankedBarChart(title, labels, a);
    case ChartKind::MetricLine:
        if (includeSummaryPoint) {
            appendSummaryLabel(labels);
            appendSummaryValue(a);
        }
        return makeSingleLineChart(title, labels, a);
    case ChartKind::CompareBar: {
        QStringList labelsB;
        b = metricSeriesValues(data, request.metricB, &labelsB, months, request.accountFilter);
        if (includeSummaryPoint) {
            appendSummaryLabel(labels);
            appendSummaryValue(a);
            appendSummaryValue(b);
        }
        QString nameA = request.seriesA.isEmpty() ? metricDisplayName(request.metricA) : request.seriesA;
        QString nameB = request.seriesB.isEmpty() ? metricDisplayName(request.metricB) : request.seriesB;
        if (usePercentBase) {
            QList<double> baseSeries;
            if (request.comparePieBaseMetric == request.metricA) baseSeries = a;
            else if (request.comparePieBaseMetric == request.metricB) baseSeries = b;
            if (!baseSeries.isEmpty()) {
                nameA += QStringLiteral(" (") + percentText(computeTotalPercentageAgainstBase(a, baseSeries)) + QStringLiteral(")");
                nameB += QStringLiteral(" (") + percentText(computeTotalPercentageAgainstBase(b, baseSeries)) + QStringLiteral(")");
                a = computePercentagesAgainstBase(a, baseSeries);
                b = computePercentagesAgainstBase(b, baseSeries);
                title += QStringLiteral(" — ") + metricDisplayName(request.comparePieBaseMetric) + QStringLiteral(" = 100%");
            }
        } else {
            const double totalAB = totalAbsValue(a) + totalAbsValue(b);
            if (totalAB > 0.000001) {
                nameA += QStringLiteral(" (") + percentText((totalAbsValue(a) / totalAB) * 100.0) + QStringLiteral(")");
                nameB += QStringLiteral(" (") + percentText((totalAbsValue(b) / totalAB) * 100.0) + QStringLiteral(")");
            }
        }
        return makeCompareBarChart(title, labels, a, b, nameA, nameB);
    }
    case ChartKind::CompareLine: {
        QStringList labelsB;
        b = metricSeriesValues(data, request.metricB, &labelsB, months, request.accountFilter);
        if (includeSummaryPoint) {
            appendSummaryLabel(labels);
            appendSummaryValue(a);
            appendSummaryValue(b);
        }
        QString nameA = request.seriesA.isEmpty() ? metricDisplayName(request.metricA) : request.seriesA;
        QString nameB = request.seriesB.isEmpty() ? metricDisplayName(request.metricB) : request.seriesB;
        if (usePercentBase) {
            QList<double> baseSeries;
            if (request.comparePieBaseMetric == request.metricA) baseSeries = a;
            else if (request.comparePieBaseMetric == request.metricB) baseSeries = b;
            if (!baseSeries.isEmpty()) {
                nameA += QStringLiteral(" (") + percentText(computeTotalPercentageAgainstBase(a, baseSeries)) + QStringLiteral(")");
                nameB += QStringLiteral(" (") + percentText(computeTotalPercentageAgainstBase(b, baseSeries)) + QStringLiteral(")");
                a = computePercentagesAgainstBase(a, baseSeries);
                b = computePercentagesAgainstBase(b, baseSeries);
                title += QStringLiteral(" — ") + metricDisplayName(request.comparePieBaseMetric) + QStringLiteral(" = 100%");
            }
        } else {
            const double totalAB = totalAbsValue(a) + totalAbsValue(b);
            if (totalAB > 0.000001) {
                nameA += QStringLiteral(" (") + percentText((totalAbsValue(a) / totalAB) * 100.0) + QStringLiteral(")");
                nameB += QStringLiteral(" (") + percentText((totalAbsValue(b) / totalAB) * 100.0) + QStringLiteral(")");
            }
        }
        return makeMultiCompareLineChart(title, labels, QList<QList<double>>{a, b}, QStringList{nameA, nameB});
    }
    case ChartKind::ComparePie: {
        QStringList labelsB;
        b = metricSeriesValues(data, request.metricB, &labelsB, months, request.accountFilter);
        double va = 0.0, vb = 0.0;
        for (double x : a) va += qAbs(x);
        for (double x : b) vb += qAbs(x);
        QStringList pieLabels;
        QList<double> pieValues;
        const QStringList names{
            request.seriesA.isEmpty() ? metricDisplayName(request.metricA) : request.seriesA,
            request.seriesB.isEmpty() ? metricDisplayName(request.metricB) : request.seriesB
        };
        const QList<double> totals{va, vb};
        const int baseIdx = (request.comparePieBaseMetric == request.metricA) ? 0 : (request.comparePieBaseMetric == request.metricB ? 1 : -1);
        buildComparePieSlices(names, totals, baseIdx, pieLabels, pieValues);
        return makeMultiComparePieChart(title, pieLabels, pieValues, 100.0);
    }
    }
    return nullptr;
}



#include "Resultswidget.moc"
