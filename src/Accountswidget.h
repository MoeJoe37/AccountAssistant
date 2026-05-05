#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVector>
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

private:
    struct RowWidgets {
        QWidget* row{};
        QLabel* accountLabel{};
        QLabel* amountCaption{};
        QLabel* typeCaption{};
        QDoubleSpinBox* amount{};
        QComboBox* type{};
        int accountIndex = -1;
    };

    void buildUi();
    void initializeMonthData();
    void syncRowsToCurrentMonth() const;
    void renderCurrentMonth();
    void clearRows();
    void updateRowTexts();
    void setComboToAccountType(QComboBox* combo, AccountType type) const;
    AccountType accountTypeFromCombo(const QComboBox* combo) const;
    QString accountTypeLabel(AccountType type) const;
    AccountTypeFilter currentGroupFilter() const;
    QString nextCustomAccountCode() const;
    void deleteAccountAtIndex(int accountIndex);

    QComboBox*   m_monthCombo{};
    QComboBox*   m_groupCombo{};
    QLabel*      m_title{};
    QLabel*      m_subtitle{};
    QLabel*      m_monthLabel{};
    QLabel*      m_groupLabel{};
    QLabel*      m_accountHeader{};
    QLabel*      m_amountHeader{};
    QLabel*      m_typeHeader{};
    QScrollArea* m_scroll{};
    QGridLayout*  m_tableHeaderLayout{};
    QWidget*     m_container{};
    QVBoxLayout* m_rowsLayout{};

    mutable std::array<QList<AccountItem>, 12> m_monthlyAccounts{};
    QVector<RowWidgets> m_rows;
    int  m_currentMonth = 0;
    bool m_loadingRows = false;
};
