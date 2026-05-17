#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolButton>
#include <QVector>
#include <QEvent>
#include <array>
#include "appdata.h"

class Accountswidget : public QWidget
{
    Q_OBJECT
public:
    explicit Accountswidget(QWidget* parent = nullptr);

    AppData collectData() const;
    void setData(const AppData& data);
    void clearData();
    void applyTheme();
    void retranslate();
    bool showGraphSelectionForRequest(const ChartRequest& request);
    void addAccount();

signals:
    void graphRequested(const ChartRequest& request);
    void dataChanged();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    struct RowWidgets {
        QWidget* row{};
        QLabel* accountLabel{};
        QLabel* accountCaption{};
        QLabel* amountCaption{};
        QLabel* typeCaption{};
        QDoubleSpinBox* amount{};
        QComboBox* type{};
        int monthIndex = -1;
        int accountIndex = -1;
    };

    struct MonthWidgets {
        QFrame* card{};
        QWidget* header{};
        QLabel* monthLabel{};
        QLabel* chevron{};
        QWidget* content{};
        QLabel* accountHeader{};
        QLabel* amountHeader{};
        QLabel* typeHeader{};
        QVBoxLayout* rowsLayout{};
        QVector<RowWidgets> rows;
        bool expanded{false};
        int monthIndex{-1};
    };

    void buildUi();
    void initializeMonthData();
    void syncRowsToCurrentMonth() const;
    void renderCurrentMonth();
    void clearRows();
    void updateRowTexts();
    void updateMonthCardText(MonthWidgets& card);
    void setMonthExpanded(int month, bool expanded);
    void setComboToAccountType(QComboBox* combo, AccountType type) const;
    AccountType accountTypeFromCombo(const QComboBox* combo) const;
    QString accountTypeLabel(AccountType type) const;
    AccountTypeFilter currentGroupFilter() const;
    QString nextCustomAccountCode() const;
    void deleteAccountAtIndex(int monthIndex, int accountIndex);
    void onShowGraphs();

    QComboBox*   m_groupCombo{};
    QLabel*      m_title{};
    QLabel*      m_subtitle{};
    QLabel*      m_groupLabel{};
    QToolButton* m_graphBtn{};
    QScrollArea* m_scroll{};
    QHBoxLayout* m_titleRow{};
    QWidget*     m_container{};
    QVBoxLayout* m_cardsLayout{};

    mutable std::array<QList<AccountItem>, 12> m_monthlyAccounts{};
    QVector<MonthWidgets> m_monthCards;
    int  m_currentMonth = 0;
    bool m_loadingRows = false;
};
