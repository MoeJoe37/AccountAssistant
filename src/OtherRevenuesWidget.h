#pragma once

#include <QWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <array>
#include "appdata.h"

class OtherRevenuesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OtherRevenuesWidget(QWidget* parent = nullptr);

    AppData collectData() const;
    void setData(const AppData& data);
    void clearData();
    void applyTheme();
    void retranslate();
    void updateCurrencyPrefix();

signals:
    void dataChanged();

private:
    void buildUi();
    void syncCurrentMonth() const;
    void loadCurrentMonth();
    QDoubleSpinBox* makeSpin(QWidget* parent);

    QLabel* m_title{};
    QLabel* m_subtitle{};
    QLabel* m_monthLabel{};
    QComboBox* m_monthCombo{};
    QLabel* m_privilegesLabel{};
    QLabel* m_miscLabel{};
    QDoubleSpinBox* m_privileges{};
    QDoubleSpinBox* m_misc{};

    mutable std::array<OtherRevenueMonthData, 12> m_values{};
    int m_currentMonth{0};
    bool m_loading{false};
};
