#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include "appdata.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Cell widget for rows that have two sub-fields
// ─────────────────────────────────────────────────────────────────────────────
class ClassicDualSpinCell : public QWidget {
    Q_OBJECT
public:
    ClassicDualSpinCell(const QString& topLabel, const QString& botLabel, QWidget* p = nullptr);
    double topValue() const;
    double botValue() const;
    void setTopValue(double v);
    void setBotValue(double v);
    void retranslate(const QString& topLabel, const QString& botLabel);
private:
    QLabel*         m_topLbl{};
    QLabel*         m_botLbl{};
    QDoubleSpinBox* m_topSpin{};
    QDoubleSpinBox* m_botSpin{};
};

// ─────────────────────────────────────────────────────────────────────────────
//  Classic spreadsheet-style data entry table
// ─────────────────────────────────────────────────────────────────────────────
class ClassicDataTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit ClassicDataTableWidget(QWidget* parent = nullptr);

    AppData collectData() const;
    void    setData(const AppData& data);
    void    retranslate();
    void    applyTheme();
    void    clearData();
    void    updateCurrency();  // no-op (classic table doesn't use currency prefixes)
    void    setInventoryMode(InventoryMode mode);
    InventoryMode inventoryMode() const { return m_mode; }

private:
    void buildTable();
    static QDoubleSpinBox* makeSpin();

    // Row scroll + frozen label column layout
    QScrollArea* m_scroll{};
    QWidget*     m_tableBody{};

    // Frozen left column labels
    QWidget*     m_leftCol{};
    QLabel*      m_cornerSpacer{};
    QLabel*      m_lSales{};
    QLabel*      m_lSalesRet{};
    QLabel*      m_lPurch{};
    QLabel*      m_lInv{};

    // Month header labels (in scrollable area)
    QLabel*      m_monthHdr[12]{};

    // Data cells
    QWidget*             m_salesWrap[12]{};
    QWidget*             m_salesRetWrap[12]{};
    QDoubleSpinBox*      m_salesCell[12]{};
    QDoubleSpinBox*      m_salesRetCell[12]{};
    ClassicDualSpinCell* m_purchCell[12]{};
    ClassicDualSpinCell* m_invCell[12]{};
    InventoryMode m_mode{InventoryMode::Periodic};
};
