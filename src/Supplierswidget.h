#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QToolButton>
#include <QVector>
#include <QWheelEvent>
#include <QFocusEvent>
#include <QPropertyAnimation>
#include <QEvent>
#include "appdata.h"

class SupplierSpinBox : public QDoubleSpinBox {
    Q_OBJECT
public:
    explicit SupplierSpinBox(QWidget* parent = nullptr);
    void updatePrefix();
protected:
    void wheelEvent(QWheelEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
};

class SupplierMonthCard : public QFrame {
    Q_OBJECT
    Q_PROPERTY(int contentHeight READ contentHeight WRITE setContentHeight)
public:
    explicit SupplierMonthCard(int monthIndex, QWidget* parent = nullptr);

    void setRowCount(int count);
    int rowCount() const;
    QList<SupplierEntry> entries() const;
    void setEntries(const QList<SupplierEntry>& entries);
    void clearAll();
    void applyTheme();
    void retranslate();
    void updateCurrencyPrefix();
    void refreshComputedValues(const QList<SupplierEntry>* previousMonthEntries = nullptr);
    void setNamesFrom(const QStringList& names);
    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool e);
    void setAddButtonVisible(bool visible);
    int contentHeight() const;
    void setContentHeight(int h);

signals:
    void addSupplierRequested();
    void supplierNameEdited(int rowIndex, const QString& name);
    void monthChanged();
    void removeSupplierRequested(int rowIndex);

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    struct RowWidgets {
        QWidget* row{};
        QLabel* nameLabel{};
        QLabel* previousBalanceLabel{};
        QLabel* purchasesLabel{};
        QLabel* totalDebtLabel{};
        QLabel* paymentsLabel{};
        QLabel* pctPurchasesLabel{};
        QLabel* pctDebtLabel{};
        QLabel* balanceLabel{};
        QLineEdit* name{};
        SupplierSpinBox* previousBalance{};
        SupplierSpinBox* purchases{};
        SupplierSpinBox* totalDebt{};
        SupplierSpinBox* payments{};
        QLabel* pctPurchases{};
        QLabel* pctDebt{};
        QLabel* balance{};
    };

    SupplierSpinBox* makeSpin(bool readOnly = false);
    QLabel* makeResultLabel();
    void appendRow();
    void setupRowContextMenu(RowWidgets& rw, int rowIndex);
    void updateStyles();
    void toggleExpand();

    int m_monthIndex{};
    bool m_expanded{false};
    QWidget* m_header{};
    QLabel* m_monthLabel{};
    QLabel* m_chevron{};
    QPushButton* m_addBtn{};
    QWidget* m_content{};
    QWidget* m_labelsRow{};
    QGridLayout* m_labelsLayout{};
    QVBoxLayout* m_rowsLayout{};
    QVector<RowWidgets> m_rows;
    int m_fullHeight{0};
    QPropertyAnimation* m_anim{};
};

class SuppliersWidget : public QWidget {
    Q_OBJECT
public:
    explicit SuppliersWidget(QWidget* parent = nullptr);

    AppData collectData() const;
    void setData(const AppData& data);
    void clearData();
    void applyTheme();
    void retranslate();
    void updateCurrencyPrefix();
    bool showGraphSelectionForRequest(const ChartRequest& request);

signals:
    void graphRequested(const ChartRequest& request);
    void dataChanged();

private slots:
    void onAddSupplierRequested();
    void onSupplierNameEdited(int rowIndex, const QString& name);
    void onMonthChanged();
    void onRemoveSupplierRequested(int rowIndex);
    void onShowGraphs();

private:
    void ensureGlobalSupplierCount(int count);
    void refreshAllComputedValues();
    QStringList currentSupplierNames() const;
    void updateGraphButtonMenu();

    QScrollArea* m_scroll{};
    QLabel*      m_title{};
    QLabel*      m_subtitle{};
    QToolButton* m_graphBtn{};
    SupplierMonthCard* m_cards[12]{};
};
