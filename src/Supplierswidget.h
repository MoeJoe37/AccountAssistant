#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QFrame>
#include <QPropertyAnimation>
#include <QKeyEvent>
#include <QWheelEvent>
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

    QString supplierName() const;
    double purchases() const;
    double payments() const;

    void setSupplierName(const QString& v);
    void setPurchases(double v);
    void setPayments(double v);

    void clearAll();
    void setExpanded(bool e);
    bool isExpanded() const { return m_expanded; }
    void applyTheme();
    void retranslate();
    void updateCurrencyPrefix();

    int contentHeight() const;
    void setContentHeight(int h);

signals:
    void dataChanged();

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    void buildHeader();
    void buildContent();
    void updateMode();
    void updateLayoutHeight();

    SupplierSpinBox* makeSpin();
    QWidget* makeFieldRow(const QString& labelText, QWidget* input);
    QWidget* makeColumn(const QString& title, QList<QWidget*> rows);

    int m_monthIndex;
    bool m_expanded{false};

    QWidget* m_header{};
    QLabel*  m_monthLabel{};
    QLabel*  m_chevron{};
    QWidget* m_content{};
    int      m_fullHeight{0};
    QPropertyAnimation* m_anim{};

    QLineEdit* m_nameEdit{};
    SupplierSpinBox* m_purchases{};
    SupplierSpinBox* m_payments{};
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

private:
    QScrollArea* m_scroll{};
    QLabel*      m_title{};
    QLabel*      m_subtitle{};
    SupplierMonthCard* m_cards[12]{};
};
