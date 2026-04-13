#pragma once
#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVector>
#include <array>
#include "appdata.h"

class QAction;
class ChartSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChartSelectionDialog(const AppData& data, QWidget* parent = nullptr);

    std::array<ChartSel, M_COUNT> selections() const;
    QList<ChartRequest> chartRequests() const;

private:
    struct MetricRow {
        MetricId id{M_SALES};
        QFrame*    frame{};
        QCheckBox* pie{};
        QCheckBox* candle{};
        QToolButton* monthsBtn{};
        QMenu*     monthsMenu{};
        QVector<QAction*> monthActions;
        QPushButton* duplicateBtn{};
        QPushButton* removeBtn{};
    };

    struct CompareRow {
        QFrame*      frame{};
        QCheckBox*   enabled{};
        QComboBox*   left{};
        QComboBox*   right{};
        QComboBox*   type{};
        QToolButton* monthBtn{};
        QMenu*       monthMenu{};
        QVector<QAction*> monthActions;
        QLineEdit*   title{};
    };

    QVector<MetricRow>   m_metricRows;
    QVector<CompareRow>   m_compareRows;
    QVBoxLayout*          m_metricLayout{};
    QVBoxLayout*          m_compareLayout{};

    void buildUI(const AppData& data);
    void appendMetricRow(MetricId id, const MetricRow* cloneFrom = nullptr, int insertAt = -1);
    void removeMetricRow(int rowIndex);
    void syncMonthButton(MetricRow& row);
    QList<int> selectedMonths(const MetricRow& row) const;
    void appendCompareRow();
    void removeCompareRow(int rowIndex);
    QList<int> selectedMonths(const CompareRow& row) const;
};
