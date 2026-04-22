#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QChartView>
#include <QList>
#include "appdata.h"

class ChartsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartsWidget(QWidget* parent = nullptr);
    void buildCharts(const AppData& data);
    void clear();

    // Returns a list of all visible chart views (for PDF export)
    QList<QChartView*> chartViews() const { return m_views; }

private:
    QScrollArea* m_scroll{nullptr};
    QWidget*     m_container{nullptr};
    QHBoxLayout* m_layout{nullptr};
    QList<QChartView*> m_views;

    void addPieChart(const QString& title,
                     const QStringList& labels,
                     const QList<double>& values,
                     MetricId metricId);

    void addCandleChart(const QString& title,
                        const QStringList& labels,
                        const QList<double>& values,
                        MetricId metricId);

    void addBarChart(const QString& title,
                     const QStringList& labels,
                     const QList<double>& values);

    void addLineChart(const QString& title,
                      const QStringList& labels,
                      const QList<double>& values,
                      MetricId metricId);

    static QChartView* makePie(const QString& title,
                               const QStringList& labels,
                               const QList<double>& values,
                               MetricId metricId);

    static QChartView* makeCandle(const QString& title,
                                  const QStringList& labels,
                                  const QList<double>& values,
                                  MetricId metricId);

    static QChartView* makeBar(const QString& title,
                               const QStringList& labels,
                               const QList<double>& values);

    static QChartView* makeLine(const QString& title,
                                const QStringList& labels,
                                const QList<double>& values,
                                MetricId metricId);
};
