#include "ClassicDataTableWidget.h"
#include "translations.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollBar>
#include <QSizePolicy>
#include <QFont>
#include <QFrame>
#include <QPalette>

// ─── colour / size constants ─────────────────────────────────────────────────
static const int CROW_H_SINGLE = 52;
static const int CROW_H_DOUBLE = 90;
static const int CCOL_W        = 168;
static const int CLABEL_W      = 150;
static const int CHDR_H        = 40;

static const char* cRowLabelSSDk =
    "background:#1c2033;"
    "color:#7b8ab8;"
    "font-weight:700;"
    "padding:0 14px;"
    "border-right:2px solid #2e3455;"
    "border-bottom:1px solid #252b42;";
static const char* cRowLabelSSLt =
    "background:#f7f9fe;"
    "color:#5a6490;"
    "font-weight:700;"
    "padding:0 14px;"
    "border-right:2px solid #dce2f2;"
    "border-bottom:1px solid #dce2f2;";
static const char* cHdrCellSSDk =
    "background:#1a1f38;"
    "color:#4f86f7;"
    "font-weight:800;"
    "padding:0 10px;"
    "border-right:1px solid #252b42;"
    "border-bottom:2px solid #4f86f7;";
static const char* cHdrCellSSLt =
    "background:#f6f8fe;"
    "color:#4f86f7;"
    "font-weight:800;"
    "padding:0 10px;"
    "border-right:1px solid #dce2f2;"
    "border-bottom:2px solid #4f86f7;";
static const char* cDataCellSSDk =
    "background:#1e2340;"
    "border-right:1px solid #252b42;"
    "border-bottom:1px solid #252b42;";
static const char* cDataCellSSLt =
    "background:#ffffff;"
    "border-right:1px solid #dce2f2;"
    "border-bottom:1px solid #dce2f2;";
static const char* cSubLabelSSDk =
    "color:#5a6490; font-weight:600; background:transparent;";
static const char* cSubLabelSSLt =
    "color:#6b7280; font-weight:600; background:transparent;";
static const char* cSpinSSDark =
    "QDoubleSpinBox {"
    "  background:#252d4a; border:1px solid #3a4268; border-radius:4px;"
    "  color:#c8d0ed; padding:3px 6px;"
    "}"
    "QDoubleSpinBox:focus { border-color:#4f86f7; }"
    "QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{"
    "  background:#2e3660; border:none; width:14px;"
    "}"
    "QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{"
    "  background:#4f86f7;"
    "}";
static const char* cSpinSSLight =
    "QDoubleSpinBox {"
    "  background:#ffffff; border:1px solid #cfd7ea; border-radius:4px;"
    "  color:#1e2340; padding:3px 6px;"
    "}"
    "QDoubleSpinBox:focus { border-color:#4f86f7; }"
    "QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{"
    "  background:#f7f9fe; border:none; width:14px;"
    "}"
    "QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{"
    "  background:#4f86f7;"
    "}";
static const char* cEditSSDark =
    "QLineEdit {"
    "  background:#252d4a; border:1px solid #3a4268; border-radius:4px;"
    "  color:#c8d0ed; padding:3px 6px;"
    "}"
    "QLineEdit:focus { border-color:#4f86f7; }";
static const char* cEditSSLight =
    "QLineEdit {"
    "  background:#ffffff; border:1px solid #cfd7ea; border-radius:4px;"
    "  color:#1e2340; padding:3px 6px;"
    "}"
    "QLineEdit:focus { border-color:#4f86f7; }";

// ═════════════════════════════════════════════════════════════════════════════
//  ClassicDualSpinCell
// ═════════════════════════════════════════════════════════════════════════════
ClassicDualSpinCell::ClassicDualSpinCell(const QString& topLabel,
                                         const QString& botLabel, QWidget* p)
    : QWidget(p)
{
    setStyleSheet(g_lightMode ? cDataCellSSLt : cDataCellSSDk);
    auto* vl = new QVBoxLayout(this);
    vl->setContentsMargins(8,6,8,6);
    vl->setSpacing(4);

    auto makeRow = [&](QLabel*& lbl, QDoubleSpinBox*& spin, const QString& text) {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        lbl = new QLabel(text);
        lbl->setStyleSheet(g_lightMode ? cSubLabelSSLt : cSubLabelSSDk);
        lbl->setFixedWidth(58);
        spin = new QDoubleSpinBox;
        spin->setRange(0, 1e10);
        spin->setDecimals(2);
        spin->setSingleStep(100);
        spin->setStyleSheet(g_lightMode ? cSpinSSLight : cSpinSSDark);
        spin->setMaximumWidth(88);
        row->addWidget(lbl);
        row->addWidget(spin);
        row->addStretch();
        vl->addLayout(row);
    };
    makeRow(m_topLbl, m_topSpin, topLabel);
    makeRow(m_botLbl, m_botSpin, botLabel);
}

double ClassicDualSpinCell::topValue() const { return m_topSpin->value(); }
double ClassicDualSpinCell::botValue() const { return m_botSpin->value(); }
void ClassicDualSpinCell::setTopValue(double v) { m_topSpin->setValue(v); }
void ClassicDualSpinCell::setBotValue(double v) { m_botSpin->setValue(v); }
void ClassicDualSpinCell::retranslate(const QString& t, const QString& b)
{
    m_topLbl->setText(t);
    m_botLbl->setText(b);
}

// ═════════════════════════════════════════════════════════════════════════════
//  ClassicExpenseCell
// ═════════════════════════════════════════════════════════════════════════════
ClassicExpenseCell::ClassicExpenseCell(QWidget* p) : QWidget(p)
{
    setStyleSheet(g_lightMode ? cDataCellSSLt : cDataCellSSDk);
    auto* vl = new QVBoxLayout(this);
    vl->setContentsMargins(8,6,8,6);
    vl->setSpacing(4);

    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        m_nameLbl = new QLabel;
        m_nameLbl->setStyleSheet(g_lightMode ? cSubLabelSSLt : cSubLabelSSDk);
        m_nameLbl->setFixedWidth(46);
        m_nameEdit = new QLineEdit;
        m_nameEdit->setStyleSheet(g_lightMode ? cEditSSLight : cEditSSDark);
        m_nameEdit->setMaximumWidth(96);
        row->addWidget(m_nameLbl);
        row->addWidget(m_nameEdit);
        row->addStretch();
        vl->addLayout(row);
    }
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(6);
        m_amtLbl = new QLabel;
        m_amtLbl->setStyleSheet(g_lightMode ? cSubLabelSSLt : cSubLabelSSDk);
        m_amtLbl->setFixedWidth(46);
        m_amtSpin = new QDoubleSpinBox;
        m_amtSpin->setRange(0, 1e10);
        m_amtSpin->setDecimals(2);
        m_amtSpin->setSingleStep(100);
        m_amtSpin->setStyleSheet(g_lightMode ? cSpinSSLight : cSpinSSDark);
        m_amtSpin->setMaximumWidth(96);
        row->addWidget(m_amtLbl);
        row->addWidget(m_amtSpin);
        row->addStretch();
        vl->addLayout(row);
    }
}

QString ClassicExpenseCell::accountName() const { return m_nameEdit->text(); }
double  ClassicExpenseCell::amount()       const { return m_amtSpin->value(); }
void ClassicExpenseCell::retranslate(const QString& nameLbl, const QString& amtLbl)
{
    m_nameLbl->setText(nameLbl);
    m_amtLbl->setText(amtLbl);
}

// ═════════════════════════════════════════════════════════════════════════════
//  ClassicDataTableWidget
// ═════════════════════════════════════════════════════════════════════════════
ClassicDataTableWidget::ClassicDataTableWidget(QWidget* parent) : QWidget(parent)
{
    buildTable();
    applyTheme();
}

QDoubleSpinBox* ClassicDataTableWidget::makeSpin()
{
    auto* s = new QDoubleSpinBox;
    s->setRange(0, 1e10);
    s->setDecimals(2);
    s->setSingleStep(100);
    s->setMaximumWidth(110);
    return s;
}

void ClassicDataTableWidget::buildTable()
{
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // ── LEFT: frozen row labels ───────────────────────────────────────────
    m_leftCol = new QWidget;
    m_leftCol->setFixedWidth(CLABEL_W);
    m_leftCol->setAutoFillBackground(true);
    m_leftCol->setStyleSheet(g_lightMode ? "background:#f7f9fe;" : "background:#1c2033;");
    auto* leftVL = new QVBoxLayout(m_leftCol);
    leftVL->setContentsMargins(0,0,0,0);
    leftVL->setSpacing(0);

    auto makeLabel = [&](QLabel*& lbl, const QString& txt, int h) {
        lbl = new QLabel(txt);
        lbl->setFixedSize(CLABEL_W, h);
        lbl->setStyleSheet(g_lightMode ? cRowLabelSSLt : cRowLabelSSDk);
        lbl->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        leftVL->addWidget(lbl);
    };

    m_cornerSpacer = new QLabel;
    m_cornerSpacer->setFixedSize(CLABEL_W, CHDR_H);
    m_cornerSpacer->setAttribute(Qt::WA_StyledBackground, true);
    m_cornerSpacer->setAutoFillBackground(true);
    m_cornerSpacer->setStyleSheet(g_lightMode
        ? "background:#f6f8fe; border-bottom:2px solid #4f86f7;"
        : "background:#1a1f38; border-bottom:2px solid #4f86f7;");
    leftVL->addWidget(m_cornerSpacer);

    makeLabel(m_lSales,    "", CROW_H_SINGLE);
    makeLabel(m_lSalesRet, "", CROW_H_SINGLE);
    makeLabel(m_lPurch,    "", CROW_H_DOUBLE);
    makeLabel(m_lExp,      "", CROW_H_DOUBLE);
    makeLabel(m_lInv,      "", CROW_H_DOUBLE);
    leftVL->addStretch();
    root->addWidget(m_leftCol);

    // ── RIGHT: scrollable month columns ───────────────────────────────────
    m_scroll = new QScrollArea;
    m_scroll->setAutoFillBackground(true);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setWidgetResizable(false);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setStyleSheet(g_lightMode ? "background:#f7f9fe; border:none;" : "background:#181d30; border:none;");

    m_tableBody = new QWidget;
    m_tableBody->setAutoFillBackground(true);
    m_tableBody->setStyleSheet(g_lightMode ? "background:#f7f9fe;" : "background:#181d30;");
    auto* grid = new QGridLayout(m_tableBody);
    grid->setContentsMargins(0,0,0,0);
    grid->setSpacing(0);

    QStringList months = monthNames();

    for (int col = 0; col < 12; ++col) {
        m_monthHdr[col] = new QLabel(months[col]);
        m_monthHdr[col]->setFixedSize(CCOL_W, CHDR_H);
        m_monthHdr[col]->setStyleSheet(g_lightMode ? cHdrCellSSLt : cHdrCellSSDk);
        m_monthHdr[col]->setAlignment(Qt::AlignCenter);
        grid->addWidget(m_monthHdr[col], 0, col);

        // Sales
        {
            auto* cell = new QWidget;
            cell->setAutoFillBackground(true);
            cell->setFixedSize(CCOL_W, CROW_H_SINGLE);
            cell->setStyleSheet(g_lightMode ? cDataCellSSLt : cDataCellSSDk);
            auto* vl = new QVBoxLayout(cell);
            vl->setContentsMargins(10,0,10,0);
            vl->setAlignment(Qt::AlignCenter);
            m_salesWrap[col] = cell;
            m_salesCell[col] = makeSpin();
            vl->addWidget(m_salesCell[col]);
            grid->addWidget(cell, 1, col);
        }
        // Sales Return
        {
            auto* cell = new QWidget;
            cell->setAutoFillBackground(true);
            cell->setFixedSize(CCOL_W, CROW_H_SINGLE);
            cell->setStyleSheet(g_lightMode ? cDataCellSSLt : cDataCellSSDk);
            auto* vl = new QVBoxLayout(cell);
            vl->setContentsMargins(10,0,10,0);
            vl->setAlignment(Qt::AlignCenter);
            m_salesRetWrap[col] = cell;
            m_salesRetCell[col] = makeSpin();
            vl->addWidget(m_salesRetCell[col]);
            grid->addWidget(cell, 2, col);
        }
        // Purchases (dual)
        m_purchCell[col] = new ClassicDualSpinCell("","");
        m_purchCell[col]->setFixedSize(CCOL_W, CROW_H_DOUBLE);
        grid->addWidget(m_purchCell[col], 3, col);

        // Expenses
        m_expCell[col] = new ClassicExpenseCell;
        m_expCell[col]->setFixedSize(CCOL_W, CROW_H_DOUBLE);
        grid->addWidget(m_expCell[col], 4, col);

        // Inventory (dual)
        m_invCell[col] = new ClassicDualSpinCell("","");
        m_invCell[col]->setFixedSize(CCOL_W, CROW_H_DOUBLE);
        grid->addWidget(m_invCell[col], 5, col);
    }

    int totalW = CCOL_W * 12;
    m_tableBody->setFixedWidth(totalW);
    int totalH = CHDR_H + CROW_H_SINGLE*2 + CROW_H_DOUBLE*3;
    m_tableBody->setFixedHeight(totalH);

    m_scroll->setWidget(m_tableBody);
    if (m_scroll->viewport()) m_scroll->viewport()->setAutoFillBackground(true);

    // AlignTop: prevents Qt from centering the fixed-height scroll area
    // vertically when the parent (QStackedWidget) is taller than needed.
    root->addWidget(m_scroll, 1, Qt::AlignTop);
    retranslate();
}

void ClassicDataTableWidget::applyTheme()
{
    m_leftCol->setAttribute(Qt::WA_StyledBackground, true);
    m_leftCol->setAutoFillBackground(true);
    m_leftCol->setStyleSheet(g_lightMode ? "background:#ffffff;" : "background:#1c2033;");
    if (m_cornerSpacer) {
        m_cornerSpacer->setAttribute(Qt::WA_StyledBackground, true);
        m_cornerSpacer->setAutoFillBackground(true);
        m_cornerSpacer->setStyleSheet(g_lightMode
            ? "background:#f6f8fe; border-bottom:2px solid #4f86f7;"
            : "background:#1a1f38; border-bottom:2px solid #4f86f7;");
    }
    m_scroll->setStyleSheet(g_lightMode ? "background:#ffffff; border:none;" : "background:#181d30; border:none;");
    if (m_scroll->viewport())
        m_scroll->viewport()->setStyleSheet(g_lightMode ? "background:#ffffff;" : "background:#181d30;");
    m_tableBody->setStyleSheet(g_lightMode ? "background:#ffffff;" : "background:#181d30;");

    const char* rowLabelSS = g_lightMode ? cRowLabelSSLt : cRowLabelSSDk;
    const char* hdrSS      = g_lightMode ? cHdrCellSSLt  : cHdrCellSSDk;
    const char* dataSS     = g_lightMode ? cDataCellSSLt : cDataCellSSDk;
    const char* subLblSS   = g_lightMode ? cSubLabelSSLt : cSubLabelSSDk;
    const char* spinSS     = g_lightMode ? cSpinSSLight  : cSpinSSDark;
    const char* editSS     = g_lightMode ? cEditSSLight  : cEditSSDark;

    if (m_lSales)    m_lSales->setStyleSheet(rowLabelSS);
    if (m_lSalesRet) m_lSalesRet->setStyleSheet(rowLabelSS);
    if (m_lPurch)    m_lPurch->setStyleSheet(rowLabelSS);
    if (m_lExp)      m_lExp->setStyleSheet(rowLabelSS);
    if (m_lInv)      m_lInv->setStyleSheet(rowLabelSS);

    for (int i = 0; i < 12; ++i) {
        if (m_monthHdr[i]) m_monthHdr[i]->setStyleSheet(hdrSS);
        if (m_salesWrap[i]) {
            m_salesWrap[i]->setAttribute(Qt::WA_StyledBackground, true);
            m_salesWrap[i]->setAutoFillBackground(true);
            m_salesWrap[i]->setStyleSheet(dataSS);
        }
        if (m_salesRetWrap[i]) {
            m_salesRetWrap[i]->setAttribute(Qt::WA_StyledBackground, true);
            m_salesRetWrap[i]->setAutoFillBackground(true);
            m_salesRetWrap[i]->setStyleSheet(dataSS);
        }
        if (m_salesCell[i])    m_salesCell[i]->setStyleSheet(spinSS);
        if (m_salesRetCell[i]) m_salesRetCell[i]->setStyleSheet(spinSS);

        if (m_purchCell[i]) {
            m_purchCell[i]->setStyleSheet(dataSS);
            for (auto* l : m_purchCell[i]->findChildren<QLabel*>()) l->setStyleSheet(subLblSS);
            for (auto* s : m_purchCell[i]->findChildren<QDoubleSpinBox*>()) s->setStyleSheet(spinSS);
        }
        if (m_expCell[i]) {
            m_expCell[i]->setStyleSheet(dataSS);
            for (auto* e : m_expCell[i]->findChildren<QLineEdit*>()) e->setStyleSheet(editSS);
            for (auto* s : m_expCell[i]->findChildren<QDoubleSpinBox*>()) s->setStyleSheet(spinSS);
            for (auto* l : m_expCell[i]->findChildren<QLabel*>()) l->setStyleSheet(subLblSS);
        }
        if (m_invCell[i]) {
            m_invCell[i]->setStyleSheet(dataSS);
            for (auto* l : m_invCell[i]->findChildren<QLabel*>()) l->setStyleSheet(subLblSS);
            for (auto* s : m_invCell[i]->findChildren<QDoubleSpinBox*>()) s->setStyleSheet(spinSS);
        }
    }
}

void ClassicDataTableWidget::retranslate()
{
    QStringList months = monthNames();

    if (m_lSales)    m_lSales->setText(   T("Sales",        "\u0627\u0644\u0645\u0628\u064a\u0639\u0627\u062a"));
    if (m_lSalesRet) m_lSalesRet->setText(T("Sales Return", "\u0645\u0631\u062a\u062c\u0639\u0627\u062a"));
    if (m_lPurch)    m_lPurch->setText(   T("Purchases",    "\u0627\u0644\u0645\u0634\u062a\u0631\u064a\u0627\u062a"));
    if (m_lExp)      m_lExp->setText(     T("Expenses",     "\u0627\u0644\u0645\u0635\u0631\u0648\u0641\u0627\u062a"));
    if (m_lInv)      m_lInv->setText(     T("Inventory",    "\u0627\u0644\u0645\u062e\u0632\u0648\u0646"));

    for (int i = 0; i < 12; ++i) {
        if (m_monthHdr[i]) m_monthHdr[i]->setText(months[i]);
        if (m_purchCell[i])
            m_purchCell[i]->retranslate(
                T("Purch.", "\u0634\u0631\u0627\u0621"),
                T("Paym.",  "\u062f\u0641\u0639"));
        if (m_expCell[i])
            m_expCell[i]->retranslate(
                T("Acct.", "\u062d\u0633\u0627\u0628"),
                T("Amt.",  "\u0645\u0628\u0644\u063a"));
        if (m_invCell[i])
            m_invCell[i]->retranslate(
                T("First period", "\u0627\u0644\u0641\u062a\u0631\u0629 \u0627\u0644\u0623\u0648\u0644\u0649"),
                T("Last period",  "\u0627\u0644\u0641\u062a\u0631\u0629 \u0627\u0644\u0623\u062e\u064a\u0631\u0629"));
    }
}

void ClassicDataTableWidget::clearData()
{
    for (int i = 0; i < 12; ++i) {
        if (m_salesCell[i])    m_salesCell[i]->setValue(0);
        if (m_salesRetCell[i]) m_salesRetCell[i]->setValue(0);
        if (m_purchCell[i]) {
            m_purchCell[i]->setTopValue(0);
            m_purchCell[i]->setBotValue(0);
        }
        if (m_expCell[i]) {
            const auto edits = m_expCell[i]->findChildren<QLineEdit*>();
            for (auto* e : edits) e->clear();
            const auto spins = m_expCell[i]->findChildren<QDoubleSpinBox*>();
            for (auto* s : spins) s->setValue(0);
        }
        if (m_invCell[i]) {
            m_invCell[i]->setTopValue(0);
            m_invCell[i]->setBotValue(0);
        }
    }
}

void ClassicDataTableWidget::updateCurrency()
{
    // Classic table doesn't show currency prefixes in spinboxes
    // (values are treated as plain numbers). No-op.
}

void ClassicDataTableWidget::setData(const AppData& d)
{
    for (int i = 0; i < 12; ++i) {
        const auto& m = d.months[i];
        if (m_salesCell[i])    m_salesCell[i]->setValue(m.sales);
        if (m_salesRetCell[i]) m_salesRetCell[i]->setValue(m.salesReturn);
        if (m_purchCell[i]) {
            m_purchCell[i]->setTopValue(m.supplierPurchases);
            m_purchCell[i]->setBotValue(m.supplierPayments);
        }
        if (m_expCell[i]) {
            const auto edits = m_expCell[i]->findChildren<QLineEdit*>();
            if (!edits.isEmpty()) edits.first()->setText(m.expenseAccount);
            const auto spins = m_expCell[i]->findChildren<QDoubleSpinBox*>();
            if (!spins.isEmpty()) spins.first()->setValue(m.expenseAmount);
        }
        if (m_invCell[i]) {
            m_invCell[i]->setTopValue(m.inventoryFirst);
            m_invCell[i]->setBotValue(m.inventoryLast);
        }
    }
}

AppData ClassicDataTableWidget::collectData() const
{
    AppData d;
    for (int i = 0; i < 12; ++i) {
        auto& m = d.months[i];
        if (m_salesCell[i])    m.sales             = m_salesCell[i]->value();
        if (m_salesRetCell[i]) m.salesReturn       = m_salesRetCell[i]->value();
        if (m_purchCell[i]) {
            m.supplierPurchases = m_purchCell[i]->topValue();
            m.supplierPayments  = m_purchCell[i]->botValue();
        }
        if (m_expCell[i]) {
            m.expenseAccount = m_expCell[i]->accountName();
            m.expenseAmount  = m_expCell[i]->amount();
        }
        if (m_invCell[i]) {
            m.inventoryFirst = m_invCell[i]->topValue();
            m.inventoryLast  = m_invCell[i]->botValue();
        }
    }
    return d;
}
