#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QVBoxLayout>
#include <QVector>
#include <QEvent>
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

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    struct MonthWidgets {
        QFrame* card{};
        QWidget* header{};
        QLabel* monthLabel{};
        QLabel* chevron{};
        QWidget* content{};
        QLabel* privilegesLabel{};
        QLabel* miscLabel{};
        QDoubleSpinBox* privileges{};
        QDoubleSpinBox* misc{};
        bool expanded{false};
        int monthIndex{-1};
    };

    void buildUi();
    void syncAllMonths() const;
    void loadAllMonths();
    void updateMonthCardText(MonthWidgets& card);
    void setMonthExpanded(int month, bool expanded);
    QDoubleSpinBox* makeSpin(QWidget* parent);

    QLabel* m_title{};
    QLabel* m_subtitle{};
    QScrollArea* m_scroll{};
    QWidget* m_container{};
    QVBoxLayout* m_cardsLayout{};

    QVector<MonthWidgets> m_monthCards;
    mutable std::array<OtherRevenueMonthData, 12> m_values{};
    bool m_loading{false};
};
