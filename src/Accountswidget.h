#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QVector>
#include <QPair>
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

signals:
    void graphRequested(ChartKind kind, AccountTypeFilter accountFilter);

private slots:
    void onAddAccount();
    void onSortChanged(int index);
    void onGroupChanged(int index);
    void onShowGraphs();
    void onRemoveRow();
    void onRowChanged();
    void onNameEdited();

private:
    struct RowWidgets {
        QWidget* row{};
        QLineEdit* name{};
        QComboBox* type{};
        QDoubleSpinBox* amount{};
        QPushButton* removeBtn{};
        QString lastValidName;
    };

    void rebuildRows(const QList<AccountItem>& items);
    QList<AccountItem> currentItems() const;
    void sortAndRebuild();
    void addRow(const QString& name = QString(), double amount = 0.0, AccountType type = AccountType::Payable);
    void updateGraphButtonMenu();
    void updatePrefixes();
    void applyGroupFilter();
    bool hasDuplicateName(const QString& name, const QLineEdit* except = nullptr) const;
    AccountTypeFilter currentGroupFilter() const;
    static AccountType accountTypeFromIndex(int index);
    static int accountTypeIndex(AccountType type);

    QScrollArea* m_scroll{};
    QWidget*     m_container{};
    QVBoxLayout* m_rowsLayout{};
    QLineEdit*   m_nameEdit{};
    QComboBox*   m_typeCombo{};
    QDoubleSpinBox* m_amountSpin{};
    QPushButton* m_addBtn{};
    QComboBox*   m_sortCombo{};
    QComboBox*   m_groupCombo{};
    QToolButton* m_graphBtn{};
    QLabel*      m_title{};
    QLabel*      m_subtitle{};
    QLabel*      m_sortLabel{};
    QLabel*      m_groupLabel{};
    QLabel*      m_empty{};
    QVector<RowWidgets> m_rows;
};
