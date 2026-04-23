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
class QLabel;
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
        QCheckBox* enabled{};
        QComboBox* type{};
        QToolButton* monthsBtn{};
        QMenu*     monthsMenu{};
        QVector<QAction*> monthActions;
        QCheckBox* summaryCheck{};
        QPushButton* duplicateBtn{};
        QPushButton* removeBtn{};
    };

    struct CompareRow {
        QFrame*      frame{};
        QVBoxLayout* layout{};
        QCheckBox*   enabled{};
        QComboBox*   left{};
        QComboBox*   right{};
        QToolButton* moreBtn{};
        QMenu*       moreMenu{};
        QVector<QAction*> moreActions;
        QList<MetricId> moreMetrics;
        QComboBox*   countAs100{};
        MetricId     comparePieBase{M_COUNT};
        QComboBox*   type{};
        QToolButton* monthBtn{};
        QMenu*       monthMenu{};
        QVector<QAction*> monthActions;
        QCheckBox*   summaryCheck{};
        QLineEdit*   title{};
    };

    QVector<MetricRow>    m_metricRows;
    QVector<CompareRow>   m_compareRows;
    QVBoxLayout*          m_metricLayout{};
    QVBoxLayout*          m_compareLayout{};
    QVBoxLayout*          m_compareGeneralLayout{};
    QVBoxLayout*          m_compareAccountsLayout{};
    QVBoxLayout*          m_compareSuppliersLayout{};
    QLabel*               m_comparePieBaseHdr{};
    CompareGroup          m_nextCompareGroup{CompareGroup::General};

    void buildUI(const AppData& data);
    void appendMetricRow(MetricId id, ChartKind kind, const QList<int>& months = {}, int insertAt = -1);
    void removeMetricRow(int rowIndex);
    void syncMonthButton(MetricRow& row);
    QList<int> selectedMonths(const MetricRow& row) const;
    void appendCompareRow(const ChartRequest* preset = nullptr);
    void duplicateCompareRow(int rowIndex);
    void removeCompareRow(int rowIndex);
    QList<int> selectedMonths(const CompareRow& row) const;
    QList<MetricId> selectedCompareMetrics(const CompareRow& row) const;
    void syncCompareMoreButton(CompareRow& row);
    void syncComparePieBaseControls(CompareRow& row);
    void syncComparePieBaseVisibility();
};
