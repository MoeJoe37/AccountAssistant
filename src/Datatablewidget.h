#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QPropertyAnimation>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QEvent>
#include "appdata.h"

class NavigableSpinBox : public QDoubleSpinBox {
    Q_OBJECT
public:
    explicit NavigableSpinBox(QWidget* p = nullptr);
    void updatePrefix();
protected:
    void keyPressEvent(QKeyEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
};

class MonthCard : public QFrame {
    Q_OBJECT
    Q_PROPERTY(int contentHeight READ contentHeight WRITE setContentHeight)
public:
    explicit MonthCard(int monthIndex, QWidget* parent = nullptr);

    double sales() const;
    double salesReturn() const;
    double supplierPurchases() const;
    double supplierPayments() const;
    double inventoryFirst() const;
    double inventoryLast() const;
    double cogsInput() const;

    void setSales(double v);
    void setSalesReturn(double v);
    void setSupplierPurchases(double v);
    void setSupplierPayments(double v);
    void setInventoryFirst(double v);
    void setInventoryLast(double v);
    void setCogsInput(double v);

    void clearAll();
    void updateCurrencyPrefix();
    void applyTheme();
    void retranslate();
    void setMode(InventoryMode mode);

    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool e);
    int contentHeight() const;
    void setContentHeight(int h);

signals:
    void dataChanged();

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    void buildHeader();
    void buildContent();
    void toggleExpand();
    void updateWarnings();
    void updateModeVisibility();

    NavigableSpinBox* makeSpin(bool redTint = false);
    QWidget* makeFieldRow(QLabel*& label, const QString& labelText, QWidget* input);
    QWidget* makeColumn(QLabel*& titleLabel, const QString& sectionTitle, QList<QWidget*> rows);
    static QString moneyText(double v);

    int m_monthIndex;
    bool m_expanded{false};
    InventoryMode m_mode{InventoryMode::Periodic};

    QWidget* m_header{};
    QLabel*  m_monthLabel{};
    QLabel*  m_warnIcon{};
    QLabel*  m_chevron{};

    QWidget* m_content{};
    int      m_fullHeight{0};

    NavigableSpinBox* m_sales{};
    NavigableSpinBox* m_salesReturn{};
    NavigableSpinBox* m_suppPurchases{};
    NavigableSpinBox* m_suppPayments{};
    NavigableSpinBox* m_invFirst{};
    NavigableSpinBox* m_invLast{};
    NavigableSpinBox* m_cogsInput{};
    QLabel*           m_netSalesValue{};
    QLabel*           m_profitValue{};

    QLabel* m_salesTitle{};
    QLabel* m_supplierTitle{};
    QLabel* m_inventoryTitle{};
    QLabel* m_ongoingTitle{};
    QLabel* m_resultsTitle{};

    QLabel* m_salesAmountLabel{};
    QLabel* m_salesReturnLabel{};
    QLabel* m_supplierPurchasesLabel{};
    QLabel* m_supplierPaymentsLabel{};
    QLabel* m_openingStockLabel{};
    QLabel* m_closingStockLabel{};
    QLabel* m_cogsInputLabel{};
    QLabel* m_netSalesLabel{};
    QLabel* m_profitMarginLabel{};

    QWidget* m_salesCol{};
    QWidget* m_supplierCol{};
    QWidget* m_inventoryCol{};
    QWidget* m_ongoingCol{};
    QWidget* m_resultsCol{};

    QFrame*  m_warnFrame{};
    QLabel*  m_warnHdr{};
    QLabel*  m_warnList{};
    QPropertyAnimation* m_anim{};
};

class DataTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit DataTableWidget(QWidget* parent = nullptr);

    AppData collectData() const;
    void setData(const AppData& data);
    void retranslate();
    void applyTheme();
    void clearData();
    void updateCurrency();
    void setInventoryMode(InventoryMode mode);
    InventoryMode inventoryMode() const { return m_mode; }

signals:
    void dataChanged();

private:
    QScrollArea* m_scroll{};
    QLabel*      m_title{};
    QLabel*      m_subtitle{};
    MonthCard*   m_cards[12]{};
    InventoryMode m_mode{InventoryMode::Periodic};
};
