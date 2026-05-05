#pragma once

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
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
    QTableWidget* m_table{};
    AppData m_data;
};
