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

// ─────────────────────────────────────────────────────────────────────────────
//  NavigableSpinBox: thousands separator + backspace fix + focus-select
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
//  MonthCard – a collapsible card for one month's data entry
// ─────────────────────────────────────────────────────────────────────────────
class MonthCard : public QFrame {
    Q_OBJECT
    Q_PROPERTY(int contentHeight READ contentHeight WRITE setContentHeight)
public:
    explicit MonthCard(int monthIndex, QWidget* parent = nullptr);

    // Data access
    double sales()             const;
    double salesReturn()       const;
    double supplierPurchases() const;
    double supplierPayments()  const;
    double inventoryFirst()    const;
    double inventoryLast()     const;

    void setSales(double v);
    void setSalesReturn(double v);
    void setSupplierPurchases(double v);
    void setSupplierPayments(double v);
    void setInventoryFirst(double v);
    void setInventoryLast(double v);

    void clearAll();
    void updateCurrencyPrefix();
    void applyTheme();
    void retranslate();

    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool e);

    // For QPropertyAnimation
    int  contentHeight() const;
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

    NavigableSpinBox* makeSpin(bool redTint = false);
    QWidget* makeFieldRow(const QString& labelText, QWidget* input);
    QWidget* makeColumn(const QString& sectionTitle, QList<QWidget*> rows);

    int          m_monthIndex;
    bool         m_expanded{false};

    // Header widgets
    QWidget*     m_header{};
    QLabel*      m_monthLabel{};
    QLabel*      m_warnIcon{};
    QLabel*      m_chevron{};

    // Content
    QWidget*     m_content{};
    int          m_fullHeight{0};

    // Fields
    NavigableSpinBox* m_sales{};
    NavigableSpinBox* m_salesReturn{};
    NavigableSpinBox* m_suppPurchases{};
    NavigableSpinBox* m_suppPayments{};
    NavigableSpinBox* m_invFirst{};
    NavigableSpinBox* m_invLast{};

    // Warnings
    QFrame*      m_warnFrame{};
    QLabel*      m_warnHdr{};
    QLabel*      m_warnList{};

    QPropertyAnimation* m_anim{};
};

// ─────────────────────────────────────────────────────────────────────────────
//  DataTableWidget – scroll area containing 12 MonthCards
// ─────────────────────────────────────────────────────────────────────────────
class DataTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit DataTableWidget(QWidget* parent = nullptr);

    AppData collectData() const;
    void    setData(const AppData& data);
    void    retranslate();
    void    applyTheme();
    void    clearData();
    void    updateCurrency();

private:
    QScrollArea* m_scroll{};
    QLabel*      m_title{};
    QLabel*      m_subtitle{};
    MonthCard*   m_cards[12]{};
};
