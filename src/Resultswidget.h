#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QHash>
#include <QVBoxLayout>
#include <QVector>
#include <QLabel>
#include <QToolButton>
#include <QMenu>
#include <QTableWidget>
#include "appdata.h"
#include "draggablechartcard.h"

class MonthReportCard;
class PageSeparatorCard;

class ResultsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ResultsWidget(QWidget* parent = nullptr);

    void buildResults(const AppData& data);
    void appendChart(const AppData& data, const ChartRequest& request);
    void clearResults();
    void applyTheme();
    void retranslate();

    QList<QChartView*> allChartViews() const;
    QList<ChartRequest> chartRequests() const;
    QList<int> monthOrder() const;
    QList<ResultFlowItem> flowOrder() const;
    bool pageLandscape() const { return m_pageLandscape; }

signals:
    void editChartsRequested();

private slots:
    void onSwapFlowItems(int fromIdx, int toIdx);
    void onAddSeparatorAfter(int flowIndex);
    void onRemoveSeparator(int separatorId);
    void onHideCard(int cardIndex);
    void onRestoreHidden(QAction* action);

private:
    QWidget* buildReportSection(const AppData& data);
    QWidget* buildPageBreakSection();
    QWidget* buildChartsSection();
    void rebuildMonthGrid();
    void rebuildGrid();
    void onSwapMonthCards(int fromIdx, int toIdx);
    void onSwapCards(int fromIdx, int toIdx);
    void rebuildFlow();
    void rebuildHiddenMenu();
    void addCard(const ChartRequest& request, QChartView* view);
    void ensureDefaultFlowOrder();
    void updatePageMode();
    void rebuildMonthSelectorMenu();
    void setVisibleMonths(const QList<int>& months);

    static QChartView* createChartView(const AppData& data, const ChartRequest& request);
    static QChartView* makePieChart(const QString& title,
                                    const QStringList& labels,
                                    const QList<double>& values);
    static QChartView* makeCandleChart(const QString& title,
                                       const QStringList& labels,
                                       const QList<double>& values);
    static QChartView* makeCompareCandleChart(const QString& title,
                                              const QStringList& labels,
                                              const QList<double>& seriesA,
                                              const QList<double>& seriesB,
                                              const QString& nameA,
                                              const QString& nameB);
    static QChartView* makeRankedBarChart(const QString& title,
                                          const QStringList& labels,
                                          const QList<double>& values);
    static QChartView* makeSingleLineChart(const QString& title,
                                           const QStringList& labels,
                                           const QList<double>& values);
    static QChartView* makeCompareBarChart(const QString& title,
                                           const QStringList& labels,
                                           const QList<double>& seriesA,
                                           const QList<double>& seriesB,
                                           const QString& nameA,
                                           const QString& nameB);
    static QChartView* makeCompareLineChart(const QString& title,
                                            const QStringList& labels,
                                            const QList<double>& seriesA,
                                            const QList<double>& seriesB,
                                            const QString& nameA,
                                            const QString& nameB);
    static QChartView* makeComparePieChart(const QString& title,
                                           const QString& nameA,
                                           const QString& nameB,
                                           double valueA,
                                           double valueB);

    static void applyChartTheme(QChart* chart, const QString& title);

    QWidget* m_summaryBar{};
    QLabel* m_sumNetSalesTitle{};
    QLabel* m_sumNetSales{};
    QLabel* m_sumCOGSTitle{};
    QLabel* m_sumCOGS{};
    QLabel* m_sumProfitTitle{};
    QLabel* m_sumProfit{};
    QToolButton* m_hiddenBtn{};
    QMenu* m_hiddenMenu{};
    QToolButton* m_monthBtn{};
    QMenu* m_monthMenu{};
    QToolButton* m_orientBtn{};
    QMenu* m_orientMenu{};
    QLabel* m_reportTitle{};
    QLabel* m_reportSub{};
    QLabel* m_pageBreakLabel{};
    QLabel* m_chartsTitle{};
    QLabel* m_chartsSub{};
    QLabel* m_flowTitle{};
    QLabel* m_flowSub{};

    QScrollArea* m_scroll{};
    QWidget* m_container{};
    QVBoxLayout* m_contentLayout{};
    QWidget* m_monthSection{};
    QGridLayout* m_monthGrid{};
    QWidget* m_pageBreakSection{};
    QWidget* m_chartsSection{};
    QGridLayout* m_grid{};
    QLabel* m_monthEmpty{};
    QLabel* m_emptyState{};
    QWidget* m_flowSection{};
    QVBoxLayout* m_flowLayout{};
    QLabel* m_flowEmpty{};

    QVector<int> m_monthOrder;
    QVector<MonthReportCard*> m_monthCards;
    QList<int> m_visibleMonths;
    bool m_pageLandscape{true};
    QVector<DraggableChartCard*> m_cards;
    QVector<ChartRequest> m_cardRequests;
    QVector<DraggableChartCard*> m_hiddenCards;
    QVector<ChartRequest> m_hiddenRequests;
    QVector<ResultFlowItem> m_flowOrder;
    QHash<int, PageSeparatorCard*> m_separatorCards;
    int m_nextSeparatorId{0};
};
