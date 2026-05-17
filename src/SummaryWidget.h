#pragma once

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QToolButton>
#include <QMenu>
#include <QAction>
#include <QVector>
#include "appdata.h"

class SummaryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SummaryWidget(QWidget* parent = nullptr);

    void setData(const AppData& data);
    void clearData();
    void applyTheme();
    void retranslate();

private:
    void buildUi();
    void rebuildTable();
    void updateCards();
    void buildMonthMenu();
    void updateMonthButton();
    QList<int> selectedMonths() const;
    QList<int> selectedMonthsFromMenu() const;

    QLabel* m_title{};
    QLabel* m_subtitle{};
    QLabel* m_tradingTitle{};
    QLabel* m_tradingValue{};
    QLabel* m_otherTitle{};
    QLabel* m_otherValue{};
    QLabel* m_expensesTitle{};
    QLabel* m_expensesValue{};
    QLabel* m_operatingTitle{};
    QLabel* m_operatingValue{};
    QLabel* m_tableTitle{};
    QLabel* m_monthLabel{};
    QToolButton* m_monthBtn{};
    QMenu* m_monthMenu{};
    QVector<QAction*> m_monthActions;
    QList<int> m_selectedMonths;
    QTableWidget* m_table{};
    AppData m_data;
};
