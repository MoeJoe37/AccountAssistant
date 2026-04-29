#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QVector>
#include <QList>
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

signals:
    void graphRequested(const ChartRequest& request);

private slots:
    void onAddAccount();
    void onFilterChanged();
    void onShowGraphs();
    void onRemoveRow();
    void onEditRow();

private:
    struct RowWidgets {
        QWidget* row{};
        QLabel*  code{};
        QLabel*  name{};
        QLabel*  type{};
        QLabel*  settlement{};
        QLabel*  currency{};
        QLabel*  amountLabel{};
        QDoubleSpinBox* amount{};
        QPushButton* removeBtn{};
        AccountItem item;
        int sourceIndex = -1;
    };

    void rebuildRows(const QList<AccountItem>& items);
    QList<AccountItem> currentItems() const;
    void sortAndRebuild();
    void addRow(const AccountItem& item);
    void addRenderedRow(const AccountItem& item, int sourceIndex);
    bool openAccountDialog(AccountItem& item, bool editMode, int ignoreIndex = -1);
    void editAccountAt(int sourceIndex);
    void clearRenderedRows();
    void syncVisibleRowsToAccounts();
    QList<int> filteredAccountIndexes() const;
    void renderCurrentPage();
    void updatePaginationControls(int filteredCount);
    int totalPagesForCount(int count) const;
    void updateRowAlignment(RowWidgets& row);
    bool openGraphDialog(ChartRequest& request, const ChartRequest* existing = nullptr);
    ChartKind normalizeAccountChartKind(ChartKind kind) const;
    void updateGraphButtonMenu();
    void applyFilters();
    void refreshCurrencyFilter();
    bool hasDuplicateAccount(const AccountItem& item, int ignoreIndex = -1) const;
    AccountTypeFilter currentTypeFilter() const;
    AccountType accountTypeFromCombo(const QComboBox* combo) const;
    void populateTypeCombo(QComboBox* combo, bool includeAll = false) const;

    QScrollArea* m_scroll{};
    QWidget*     m_container{};
    QVBoxLayout* m_rowsLayout{};
    QLineEdit*   m_searchEdit{};
    QPushButton* m_addBtn{};
    QComboBox*   m_sortCombo{};
    QComboBox*   m_typeFilterCombo{};
    QComboBox*   m_settlementFilterCombo{};
    QComboBox*   m_currencyFilterCombo{};
    QToolButton* m_graphBtn{};
    QLabel*      m_title{};
    QLabel*      m_subtitle{};
    QLabel*      m_searchLabel{};
    QLabel*      m_sortLabel{};
    QLabel*      m_typeLabel{};
    QLabel*      m_settlementLabel{};
    QLabel*      m_currencyLabel{};
    QLabel*      m_empty{};
    QWidget*     m_paginationBar{};
    QPushButton* m_prevPageBtn{};
    QPushButton* m_nextPageBtn{};
    QLabel*      m_pageLabel{};
    QVector<RowWidgets> m_rows;
    QList<AccountItem> m_accounts;
    int m_currentPage = 0;
};
