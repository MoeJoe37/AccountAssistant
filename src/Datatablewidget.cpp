#include "Datatablewidget.h"
#include "translations.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QFocusEvent>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QMouseEvent>
#include <QAbstractSpinBox>
#include <QLocale>
#include <QSizePolicy>

static const char* kCardDk = "QFrame#monthCard{background:#141827;border:1px solid #252b4a;border-radius:10px;}";
static const char* kCardLt = "QFrame#monthCard{background:#ffffff;border:1px solid #dde2f0;border-radius:10px;}";
static const char* kHdrDk  = "QWidget#cardHeader{background:#1a1f38;border-radius:9px 9px 0 0;}QWidget#cardHeader:hover{background:#1e2445;}";
static const char* kHdrLt  = "QWidget#cardHeader{background:#f4f6fb;border-radius:9px 9px 0 0;}QWidget#cardHeader:hover{background:#eef0fa;}";
static const char* kSecTitleDk = "color:#4f86f7;font-weight:800;background:transparent;";
static const char* kSecTitleLt = "color:#2563eb;font-weight:800;background:transparent;";
static const char* kFieldLblDk = "color:#5a6490;font-weight:600;background:transparent;";
static const char* kFieldLblLt = "color:#6b7280;font-weight:600;background:transparent;";
static const char* kMonthLblDk = "color:#c8d0ed;font-weight:700;background:transparent;";
static const char* kMonthLblLt = "color:#1e2340;font-weight:700;background:transparent;";
static const char* kChevronDk  = "color:#5a6490;background:transparent;";
static const char* kChevronLt  = "color:#8892b8;background:transparent;";
static const char* kSpinDk = "QDoubleSpinBox{background:#252d4a;border:1px solid #3a4268;border-radius:6px;color:#c8d0ed;font-family:'Consolas','Courier New',monospace;padding:4px 8px;text-align:right;}QDoubleSpinBox:focus{border-color:#4f86f7;background:#2a3255;}QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{background:#2e3660;border:none;width:16px;}QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{background:#4f86f7;}";
static const char* kSpinLt = "QDoubleSpinBox{background:#ffffff;border:1px solid #cfd7ea;border-radius:6px;color:#1e2340;font-family:'Consolas','Courier New',monospace;padding:4px 8px;text-align:right;}QDoubleSpinBox:focus{border-color:#4f86f7;background:#f8faff;}QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{background:#f7f9fe;border:none;width:16px;}QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{background:#4f86f7;}";
static const char* kSpinRedDk = "QDoubleSpinBox{background:#2a1a1a;border:1px solid #6b2a2a;border-radius:6px;color:#ff8080;font-family:'Consolas','Courier New',monospace;padding:4px 8px;text-align:right;}QDoubleSpinBox:focus{border-color:#e74c3c;background:#311a1a;}QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{background:#3a1a1a;border:none;width:16px;}QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{background:#e74c3c;}";
static const char* kSpinRedLt = "QDoubleSpinBox{background:#fff5f5;border:1px solid #f5c6c6;border-radius:6px;color:#c0392b;font-family:'Consolas','Courier New',monospace;padding:4px 8px;text-align:right;}QDoubleSpinBox:focus{border-color:#e74c3c;background:#fff0f0;}QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{background:#fff5f5;border:none;width:16px;}QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{background:#e74c3c;}";
static const char* kContentDk = "QWidget#cardContent{background:#141827;border-radius:0 0 9px 9px;}";
static const char* kContentLt = "QWidget#cardContent{background:#ffffff;border-radius:0 0 9px 9px;}";
static const char* kWarnFrameDk = "QFrame#warnFrame{background:rgba(245,158,11,0.08);border:1px solid rgba(245,158,11,0.25);border-radius:8px;}";
static const char* kWarnFrameLt = "QFrame#warnFrame{background:#fff8e6;border:1px solid #f6d48b;border-radius:8px;}";

static const char* kTableDark = R"(
QWidget#dataTableRoot { background:#0d1020; }
QWidget#dataSubHeader { background:#111526; border-bottom:1px solid #1a1f38; }
QWidget#dataTab { background:#0d1020; }
QLabel#dataHint { color:#5a6490; background:transparent; }
QLineEdit, QDoubleSpinBox, QAbstractSpinBox {
    background:#252d4a; color:#c8d0ed; border:1px solid #3a4268; border-radius:5px;
}
QLineEdit:focus, QDoubleSpinBox:focus, QAbstractSpinBox:focus { border-color:#4f86f7; }
)";
static const char* kTableLight = R"(
QWidget#dataTableRoot { background:#f4f6fb; }
QWidget#dataSubHeader { background:#ffffff; border-bottom:1px solid #dde2f0; }
QWidget#dataTab { background:#f4f6fb; }
QLabel#dataHint { color:#6b7280; background:transparent; }
QLineEdit, QDoubleSpinBox, QAbstractSpinBox {
    background:#ffffff; color:#1e2340; border:1px solid #cfd7ea; border-radius:5px;
}
QLineEdit:focus, QDoubleSpinBox:focus, QAbstractSpinBox:focus { border-color:#4f86f7; }
)";

NavigableSpinBox::NavigableSpinBox(QWidget* p) : QDoubleSpinBox(p)
{
    setRange(0, 1e12);
    setDecimals(currencyDecimals());
    setSingleStep(100);
    setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    setGroupSeparatorShown(true);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
    updatePrefix();
}

void NavigableSpinBox::updatePrefix() { setPrefix(currencyPrefix()); setSuffix(currencySuffix()); setDecimals(currencyDecimals()); }
void NavigableSpinBox::keyPressEvent(QKeyEvent* e) { QDoubleSpinBox::keyPressEvent(e); }
void NavigableSpinBox::focusInEvent(QFocusEvent* e) { QDoubleSpinBox::focusInEvent(e); selectAll(); }
void NavigableSpinBox::wheelEvent(QWheelEvent* e) { e->ignore(); }

static QString formatMoney(double v) { return formatCurrencyNumber(v); }

MonthCard::MonthCard(int monthIndex, QWidget* parent) : QFrame(parent), m_monthIndex(monthIndex)
{
    setObjectName("monthCard");
    setAttribute(Qt::WA_StyledBackground, true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setMinimumWidth(0);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);
    buildHeader();
    buildContent();
    root->addWidget(m_header);
    root->addWidget(m_content);

    m_anim = new QPropertyAnimation(this, "contentHeight", this);
    m_anim->setDuration(220);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);

    if (m_monthIndex == 0) {
        m_content->setVisible(true);
        m_expanded = true;
        m_chevron->setText("▲");
        QTimer::singleShot(0, this, [this]{ m_fullHeight = m_content->sizeHint().height(); m_content->setMaximumHeight(m_fullHeight); });
    } else {
        m_content->setVisible(false);
        m_content->setMaximumHeight(0);
    }
    applyTheme();
    retranslate();
    updateModeVisibility();
}

void MonthCard::buildHeader()
{
    m_header = new QWidget;
    m_header->setObjectName("cardHeader");
    m_header->setFixedHeight(52);
    m_header->setCursor(Qt::PointingHandCursor);
    m_header->setAttribute(Qt::WA_StyledBackground, true);

    auto* hl = new QHBoxLayout(m_header);
    hl->setContentsMargins(18,0,18,0);
    hl->setSpacing(10);
    m_monthLabel = new QLabel;
    m_warnIcon = new QLabel("⚠");
    m_warnIcon->setStyleSheet("color:#f59e0b;background:transparent;");
    m_warnIcon->setVisible(false);
    m_warnIcon->setToolTip(tr_data_warnings_exist_e01dcc());
    m_chevron = new QLabel("▼");
    hl->addWidget(m_monthLabel);
    hl->addWidget(m_warnIcon);
    hl->addStretch();
    hl->addWidget(m_chevron);
    m_header->installEventFilter(this);
}

NavigableSpinBox* MonthCard::makeSpin(bool redTint)
{
    auto* s = new NavigableSpinBox;
    s->updatePrefix();
    s->setStyleSheet(g_lightMode ? (redTint ? kSpinRedLt : kSpinLt) : (redTint ? kSpinRedDk : kSpinDk));
    return s;
}

QWidget* MonthCard::makeFieldRow(QLabel*& label, const QString& labelText, QWidget* input)
{
    auto* w = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(3);
    label = new QLabel(labelText);
    label->setObjectName("fieldLabel");
    label->setStyleSheet(g_lightMode ? kFieldLblLt : kFieldLblDk);
    vl->addWidget(label);
    vl->addWidget(input);
    return w;
}

QWidget* MonthCard::makeColumn(QLabel*& titleLabel, const QString& sectionTitle, QList<QWidget*> rows)
{
    auto* w = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(10);
    titleLabel = new QLabel(sectionTitle);
    titleLabel->setObjectName("sectionTitle");
    titleLabel->setStyleSheet(g_lightMode ? kSecTitleLt : kSecTitleDk);
    vl->addWidget(titleLabel);
    for (auto* r : rows) vl->addWidget(r);
    vl->addStretch();
    return w;
}

void MonthCard::buildContent()
{
    m_content = new QWidget;
    m_content->setObjectName("cardContent");
    m_content->setAttribute(Qt::WA_StyledBackground, true);
    auto* vl = new QVBoxLayout(m_content);
    vl->setContentsMargins(16,14,16,14);
    vl->setSpacing(12);

    auto* cols = new QHBoxLayout;
    cols->setSpacing(12);

    m_sales = makeSpin();
    m_salesReturn = makeSpin(true);
    m_suppPurchases = makeSpin();
    m_suppPayments = makeSpin();
    m_invFirst = makeSpin();
    m_invLast = makeSpin();
    m_cogsInput = makeSpin();

    auto connectSpin = [this](NavigableSpinBox* s){
        connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MonthCard::updateWarnings);
        connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MonthCard::dataChanged);
    };
    connectSpin(m_sales); connectSpin(m_salesReturn);
    connectSpin(m_suppPurchases); connectSpin(m_suppPayments);
    connectSpin(m_invFirst); connectSpin(m_invLast);
    connectSpin(m_cogsInput);

    m_salesCol = makeColumn(m_salesTitle, tr_sales_revenue_41bc0b(), {
        makeFieldRow(m_salesAmountLabel, tr_sales_amount_0a3f3e(), m_sales),
        makeFieldRow(m_salesReturnLabel, tr_sales_return_27c2fd(), m_salesReturn)
    });
    m_supplierCol = makeColumn(m_supplierTitle, tr_suppliers_7beff3(), {
        makeFieldRow(m_supplierPurchasesLabel, tr_supplier_purchases_f5a1cd(), m_suppPurchases),
        makeFieldRow(m_supplierPaymentsLabel, tr_supplier_payments_eeef31(), m_suppPayments)
    });
    m_inventoryCol = makeColumn(m_inventoryTitle, tr_inventory_22ffe2(), {
        makeFieldRow(m_openingStockLabel, tr_opening_stock_first_period_ba1057(), m_invFirst),
        makeFieldRow(m_closingStockLabel, tr_closing_stock_last_period_a0c5b2(), m_invLast)
    });
    m_ongoingCol = makeColumn(m_ongoingTitle, tr_ongoing_inventory_4f9f2c(), {
        makeFieldRow(m_cogsInputLabel, tr_cogs_input_2a1b7e(), m_cogsInput)
    });

    m_resultsCol = makeColumn(m_resultsTitle, tr_results_87ae7f(), {
        makeFieldRow(m_netSalesLabel, tr_net_sales_90f56d(), (m_netSalesValue = new QLabel("0.00"))),
        makeFieldRow(m_profitMarginLabel, tr_profit_margin_56b595(), (m_profitValue = new QLabel("0.00")))
    });
    m_netSalesValue->setStyleSheet(g_lightMode ? "font-weight:700; color:#4f86f7;" : "font-weight:700; color:#7ab0ff;");
    m_profitValue->setStyleSheet(g_lightMode ? "font-weight:700; color:#10b981;" : "font-weight:700; color:#58d69a;");

    cols->addWidget(m_salesCol);
    cols->addWidget(m_supplierCol);
    cols->addWidget(m_inventoryCol);
    cols->addWidget(m_ongoingCol);
    vl->addLayout(cols);

    m_warnFrame = new QFrame;
    m_warnFrame->setObjectName("warnFrame");
    m_warnFrame->setAttribute(Qt::WA_StyledBackground, true);
    auto* wl = new QVBoxLayout(m_warnFrame);
    wl->setContentsMargins(12,10,12,10);
    wl->setSpacing(4);
    m_warnHdr = new QLabel(QStringLiteral("⚠  ") + tr_warnings_5eb706());
    m_warnHdr->setStyleSheet("color:#f59e0b;font-weight:700;background:transparent;");
    m_warnList = new QLabel;
    m_warnList->setWordWrap(true);
    m_warnList->setStyleSheet("color:#d97706;background:transparent;");
    wl->addWidget(m_warnHdr);
    wl->addWidget(m_warnList);
    m_warnFrame->setVisible(false);
    vl->addWidget(m_warnFrame);
}

void MonthCard::setMode(InventoryMode mode)
{
    m_mode = mode;
    updateModeVisibility();
    updateWarnings();
}

void MonthCard::updateModeVisibility()
{
    const bool periodic = (m_mode == InventoryMode::Periodic);
    if (m_supplierCol) m_supplierCol->setVisible(periodic);
    if (m_inventoryCol) m_inventoryCol->setVisible(periodic);
    if (m_ongoingCol) m_ongoingCol->setVisible(!periodic);
    if (m_resultsCol) m_resultsCol->setVisible(false);

    if (m_suppPayments) {
        if (auto* row = m_suppPayments->parentWidget())
            row->setVisible(false);
    }
    if (m_suppPurchases) m_suppPurchases->setVisible(periodic);

    if (m_netSalesValue) m_netSalesValue->setText(moneyText(sales() - salesReturn()));
    if (m_profitValue) m_profitValue->setText(moneyText((sales() - salesReturn()) - (m_mode == InventoryMode::Ongoing ? cogsInput() : (inventoryFirst() + supplierPurchases() - inventoryLast()))));
    QTimer::singleShot(0, this, [this]{
        m_fullHeight = m_content->sizeHint().height();
        if (m_expanded) m_content->setMaximumHeight(m_fullHeight);
    });
}

QString MonthCard::moneyText(double v) { return formatMoney(v); }

bool MonthCard::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_header && ev->type() == QEvent::MouseButtonRelease) {
        toggleExpand();
        return true;
    }
    return QFrame::eventFilter(obj, ev);
}

void MonthCard::toggleExpand() { setExpanded(!m_expanded); }

void MonthCard::setExpanded(bool expand)
{
    if (m_expanded == expand) return;
    m_expanded = expand;
    m_chevron->setText(m_expanded ? "▲" : "▼");
    if (m_expanded) {
        m_content->setVisible(true);
        m_fullHeight = qMax(10, m_content->sizeHint().height());
        m_anim->stop();
        m_anim->setStartValue(0);
        m_anim->setEndValue(m_fullHeight);
        m_anim->start();
    } else {
        m_anim->stop();
        m_anim->setStartValue(m_content->maximumHeight());
        m_anim->setEndValue(0);
        m_anim->start();
        connect(m_anim, &QPropertyAnimation::finished, this, [this]{ if (!m_expanded) m_content->setVisible(false); }, Qt::UniqueConnection);
    }
}

int MonthCard::contentHeight() const { return m_content ? m_content->maximumHeight() : 0; }
void MonthCard::setContentHeight(int h) { if (m_content) m_content->setMaximumHeight(h); }

double MonthCard::sales() const { return m_sales->value(); }
double MonthCard::salesReturn() const { return m_salesReturn->value(); }
double MonthCard::supplierPurchases() const { return m_suppPurchases->value(); }
double MonthCard::supplierPayments() const { return m_suppPayments->value(); }
double MonthCard::inventoryFirst() const { return m_invFirst->value(); }
double MonthCard::inventoryLast() const { return m_invLast->value(); }
double MonthCard::cogsInput() const { return m_cogsInput->value(); }

void MonthCard::setSales(double v) { m_sales->setValue(v); }
void MonthCard::setSalesReturn(double v) { m_salesReturn->setValue(v); }
void MonthCard::setSupplierPurchases(double v) { m_suppPurchases->setValue(v); }
void MonthCard::setSupplierPayments(double v) { m_suppPayments->setValue(v); }
void MonthCard::setInventoryFirst(double v) { m_invFirst->setValue(v); }
void MonthCard::setInventoryLast(double v) { m_invLast->setValue(v); }
void MonthCard::setCogsInput(double v) { m_cogsInput->setValue(v); }

void MonthCard::clearAll()
{
    m_sales->setValue(0); m_salesReturn->setValue(0); m_suppPurchases->setValue(0);
    m_suppPayments->setValue(0); m_invFirst->setValue(0); m_invLast->setValue(0); m_cogsInput->setValue(0);
    if (m_netSalesValue) m_netSalesValue->setText(formatCurrencyNumber(0.0));
    if (m_profitValue) m_profitValue->setText(formatCurrencyNumber(0.0));
    m_warnFrame->setVisible(false);
    m_warnIcon->setVisible(false);
}

void MonthCard::updateCurrencyPrefix()
{
    for (auto* s : {m_sales, m_salesReturn, m_suppPurchases, m_suppPayments, m_invFirst, m_invLast, m_cogsInput})
        if (s) s->updatePrefix();
}

void MonthCard::updateWarnings()
{
    const double netSales = sales() - salesReturn();
    const double cogs = (m_mode == InventoryMode::Ongoing) ? cogsInput() : (inventoryFirst() + supplierPurchases() - inventoryLast());
    const double profit = netSales - cogs;
    if (m_netSalesValue) m_netSalesValue->setText(moneyText(netSales));
    if (m_profitValue) m_profitValue->setText(moneyText(profit));

    QStringList warnings;
    if (m_mode == InventoryMode::Periodic && inventoryLast() > inventoryFirst() + supplierPurchases())
        warnings << tr_unusual_stock_increase_closing_97f885();
    if (profit < 0)
        warnings << tr_negative_profit_margin_loss_de_87719f();

    const bool hasWarnings = !warnings.isEmpty();
    m_warnIcon->setVisible(hasWarnings && !m_expanded);
    if (hasWarnings) {
        QString html;
        for (const QString& w : warnings) html += "• " + w + "<br>";
        m_warnList->setText(html.trimmed());
        m_warnFrame->setVisible(m_expanded);
    } else {
        m_warnFrame->setVisible(false);
    }
    if (m_expanded) {
        QTimer::singleShot(0, this, [this]{
            m_fullHeight = m_content->sizeHint().height();
            m_content->setMaximumHeight(m_fullHeight);
        });
    }
}

void MonthCard::applyTheme()
{
    setStyleSheet(g_lightMode ? kCardLt : kCardDk);
    m_header->setStyleSheet(g_lightMode ? kHdrLt : kHdrDk);
    m_monthLabel->setStyleSheet(g_lightMode ? kMonthLblLt : kMonthLblDk);
    m_chevron->setStyleSheet(g_lightMode ? kChevronLt : kChevronDk);
    m_content->setStyleSheet(g_lightMode ? kContentLt : kContentDk);
    for (auto* lbl : m_content->findChildren<QLabel*>("sectionTitle")) lbl->setStyleSheet(g_lightMode ? kSecTitleLt : kSecTitleDk);
    for (auto* lbl : m_content->findChildren<QLabel*>("fieldLabel")) lbl->setStyleSheet(g_lightMode ? kFieldLblLt : kFieldLblDk);
    if (m_sales) m_sales->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    if (m_salesReturn) m_salesReturn->setStyleSheet(g_lightMode ? kSpinRedLt : kSpinRedDk);
    for (auto* s : {m_suppPurchases, m_suppPayments, m_invFirst, m_invLast, m_cogsInput})
        if (s) s->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    if (m_warnFrame) m_warnFrame->setStyleSheet(g_lightMode ? kWarnFrameLt : kWarnFrameDk);
}

void MonthCard::retranslate()
{
    const QStringList names = monthNames();
    m_monthLabel->setText(names.value(m_monthIndex));
    if (m_warnIcon) m_warnIcon->setToolTip(tr_data_warnings_exist_e01dcc());
    if (m_warnHdr) m_warnHdr->setText(QStringLiteral("⚠  ") + tr_warnings_5eb706());
    if (m_salesTitle) m_salesTitle->setText(tr_sales_revenue_41bc0b());
    if (m_supplierTitle) m_supplierTitle->setText(tr_suppliers_7beff3());
    if (m_inventoryTitle) m_inventoryTitle->setText(tr_inventory_22ffe2());
    if (m_ongoingTitle) m_ongoingTitle->setText(tr_ongoing_inventory_4f9f2c());
    if (m_resultsTitle) m_resultsTitle->setText(tr_results_87ae7f());

    if (m_salesAmountLabel) m_salesAmountLabel->setText(tr_sales_amount_0a3f3e());
    if (m_salesReturnLabel) m_salesReturnLabel->setText(tr_sales_return_27c2fd());
    if (m_supplierPurchasesLabel) m_supplierPurchasesLabel->setText(tr_supplier_purchases_f5a1cd());
    if (m_supplierPaymentsLabel) m_supplierPaymentsLabel->setText(tr_supplier_payments_eeef31());
    if (m_openingStockLabel) m_openingStockLabel->setText(tr_opening_stock_first_period_ba1057());
    if (m_closingStockLabel) m_closingStockLabel->setText(tr_closing_stock_last_period_a0c5b2());
    if (m_cogsInputLabel) m_cogsInputLabel->setText(tr_cogs_input_2a1b7e());
    if (m_netSalesLabel) m_netSalesLabel->setText(tr_net_sales_90f56d());
    if (m_profitMarginLabel) m_profitMarginLabel->setText(tr_profit_margin_56b595());
    updateModeVisibility();
}

AppData DataTableWidget::collectData() const
{
    AppData d;
    d.inventoryMode = m_mode;
    for (int i = 0; i < 12; ++i) {
        auto& m = d.months[i];
        m.sales = m_cards[i]->sales();
        m.salesReturn = m_cards[i]->salesReturn();
        m.supplierPurchases = m_cards[i]->supplierPurchases();
        m.supplierPayments = m_cards[i]->supplierPayments();
        m.inventoryFirst = m_cards[i]->inventoryFirst();
        m.inventoryLast = m_cards[i]->inventoryLast();
        m.cogsInput = m_cards[i]->cogsInput();
    }
    return d;
}

void DataTableWidget::setData(const AppData& d)
{
    m_mode = d.inventoryMode;
    for (int i = 0; i < 12; ++i) {
        const auto& m = d.months[i];
        m_cards[i]->setSales(m.sales);
        m_cards[i]->setSalesReturn(m.salesReturn);
        m_cards[i]->setSupplierPurchases(m.supplierPurchases);
        m_cards[i]->setSupplierPayments(m.supplierPayments);
        m_cards[i]->setInventoryFirst(m.inventoryFirst);
        m_cards[i]->setInventoryLast(m.inventoryLast);
        m_cards[i]->setCogsInput(m.cogsInput);
        if (i != 0) {
            const bool hasData = m.sales || m.salesReturn || m.supplierPurchases || m.supplierPayments || m.inventoryFirst || m.inventoryLast || m.cogsInput;
            if (hasData) m_cards[i]->setExpanded(true);
        }
        m_cards[i]->setMode(m_mode);
    }
}

void DataTableWidget::clearData() { for (int i = 0; i < 12; ++i) m_cards[i]->clearAll(); }
void DataTableWidget::updateCurrency() { for (int i = 0; i < 12; ++i) m_cards[i]->updateCurrencyPrefix(); }

DataTableWidget::DataTableWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("dataTableRoot");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setStyleSheet("QScrollArea{background:transparent;border:none;}");

    auto* container = new QWidget;
    container->setObjectName("cardsContainer");
    container->setAttribute(Qt::WA_StyledBackground, true);
    auto* vl = new QVBoxLayout(container);
    vl->setContentsMargins(20,16,20,24);
    vl->setSpacing(10);

    auto* titleBar = new QWidget;
    auto* tl = new QVBoxLayout(titleBar);
    tl->setContentsMargins(0,0,0,8);
    tl->setSpacing(2);
    m_title = new QLabel(tr_data_entry_e7b5c0());
    m_subtitle = new QLabel(tr_enter_monthly_figures_below_cl_e7d622());
    m_title->setStyleSheet(g_lightMode ? "color:#1e2340;font-weight:800;background:transparent;" : "color:#c8d0ed;font-weight:800;background:transparent;");
    m_subtitle->setStyleSheet("color:#5a6490;background:transparent;");
    tl->addWidget(m_title);
    tl->addWidget(m_subtitle);
    vl->addWidget(titleBar);

    for (int i = 0; i < 12; ++i) {
        m_cards[i] = new MonthCard(i, container);
        m_cards[i]->setMode(m_mode);
        vl->addWidget(m_cards[i]);
    }
    vl->addStretch();
    m_scroll->setWidget(container);
    root->addWidget(m_scroll);
    applyTheme();
    retranslate();
}

void DataTableWidget::setInventoryMode(InventoryMode mode)
{
    m_mode = mode;
    for (int i = 0; i < 12; ++i) m_cards[i]->setMode(mode);
}

void DataTableWidget::retranslate()
{
    if (m_title) m_title->setText(tr_data_entry_e7b5c0());
    if (m_subtitle) m_subtitle->setText(tr_enter_monthly_figures_below_cl_e7d622());
    for (int i = 0; i < 12; ++i) m_cards[i]->retranslate();
}

void DataTableWidget::applyTheme()
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(g_lightMode ? kTableLight : kTableDark);
    if (m_scroll) {
        m_scroll->setStyleSheet(g_lightMode
            ? "QScrollArea{background:transparent;border:none;}QScrollBar:vertical{background:transparent;width:8px;border-radius:4px;}QScrollBar::handle:vertical{background:#c8d0ed;border-radius:4px;min-height:30px;}QScrollBar::handle:vertical:hover{background:#4f86f7;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"
            : "QScrollArea{background:transparent;border:none;}QScrollBar:vertical{background:transparent;width:8px;border-radius:4px;}QScrollBar::handle:vertical{background:#2e3860;border-radius:4px;min-height:30px;}QScrollBar::handle:vertical:hover{background:#4f86f7;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}");
        if (m_scroll->viewport()) {
            m_scroll->viewport()->setAutoFillBackground(true);
            m_scroll->viewport()->setStyleSheet(g_lightMode ? "background:#f4f6fb;" : "background:#0d1020;");
        }
    }
    for (int i = 0; i < 12; ++i) m_cards[i]->applyTheme();
}

