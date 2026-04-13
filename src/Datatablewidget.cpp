#include "datatablewidget.h"
#include "translations.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QFocusEvent>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QPainter>
#include <QMouseEvent>

// ─────────────────────────────────────────────────────────────────────────────
//  Theme constants
// ─────────────────────────────────────────────────────────────────────────────

// Card outer frame
static const char* kCardDk =
    "QFrame#monthCard{"
    "  background:#141827;"
    "  border:1px solid #252b4a;"
    "  border-radius:10px;"
    "  margin:0px;"
    "}";
static const char* kCardLt =
    "QFrame#monthCard{"
    "  background:#ffffff;"
    "  border:1px solid #dde2f0;"
    "  border-radius:10px;"
    "  margin:0px;"
    "}";

// Card header
static const char* kHdrDk =
    "QWidget#cardHeader{"
    "  background:#1a1f38;"
    "  border-radius:9px 9px 0px 0px;"
    "}"
    "QWidget#cardHeader:hover{ background:#1e2445; }";
static const char* kHdrLt =
    "QWidget#cardHeader{"
    "  background:#f4f6fb;"
    "  border-radius:9px 9px 0px 0px;"
    "}"
    "QWidget#cardHeader:hover{ background:#eef0fa; }";

// Section title
static const char* kSecTitleDk = "color:#4f86f7; font-weight:800; background:transparent;";
static const char* kSecTitleLt = "color:#2563eb; font-weight:800; background:transparent;";

// Field label
static const char* kFieldLblDk = "color:#5a6490; font-weight:600; background:transparent;";
static const char* kFieldLblLt = "color:#6b7280; font-weight:600; background:transparent;";

// Month name label
static const char* kMonthLblDk = "color:#c8d0ed; font-weight:700; background:transparent;";
static const char* kMonthLblLt = "color:#1e2340; font-weight:700; background:transparent;";

// Chevron
static const char* kChevronDk  = "color:#5a6490; background:transparent;";
static const char* kChevronLt  = "color:#8892b8; background:transparent;";

// Spinbox dark
static const char* kSpinDk =
    "QDoubleSpinBox{"
    "  background:#252d4a; border:1px solid #3a4268; border-radius:6px;"
    "  color:#c8d0ed; font-family:'Consolas','Courier New',monospace;"
    "  padding:4px 8px; text-align:right;"
    "}"
    "QDoubleSpinBox:focus{ border-color:#4f86f7; background:#2a3255; }"
    "QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{ background:#2e3660; border:none; width:16px; }"
    "QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{ background:#4f86f7; }";

// Spinbox light
static const char* kSpinLt =
    "QDoubleSpinBox{"
    "  background:#ffffff; border:1px solid #cfd7ea; border-radius:6px;"
    "  color:#1e2340; font-family:'Consolas','Courier New',monospace;"
    "  padding:4px 8px; text-align:right;"
    "}"
    "QDoubleSpinBox:focus{ border-color:#4f86f7; background:#f8faff; }"
    "QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{ background:#f7f9fe; border:none; width:16px; }"
    "QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{ background:#4f86f7; }";

// Red-tinted spinbox (sales return)
static const char* kSpinRedDk =
    "QDoubleSpinBox{"
    "  background:#2a1a1a; border:1px solid #6b2a2a; border-radius:6px;"
    "  color:#ff8080; font-family:'Consolas','Courier New',monospace;"
    "  padding:4px 8px; text-align:right;"
    "}"
    "QDoubleSpinBox:focus{ border-color:#e74c3c; background:#311a1a; }"
    "QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{ background:#3a1a1a; border:none; width:16px; }"
    "QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{ background:#e74c3c; }";

static const char* kSpinRedLt =
    "QDoubleSpinBox{"
    "  background:#fff5f5; border:1px solid #f5c6c6; border-radius:6px;"
    "  color:#c0392b; font-family:'Consolas','Courier New',monospace;"
    "  padding:4px 8px; text-align:right;"
    "}"
    "QDoubleSpinBox:focus{ border-color:#e74c3c; background:#fff0f0; }"
    "QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{ background:#fff5f5; border:none; width:16px; }"
    "QDoubleSpinBox::up-button:hover,QDoubleSpinBox::down-button:hover{ background:#e74c3c; }";

// LineEdit
static const char* kEditDk =
    "QLineEdit{"
    "  background:#252d4a; border:1px solid #3a4268; border-radius:6px;"
    "  color:#c8d0ed; padding:4px 8px;"
    "}"
    "QLineEdit:focus{ border-color:#4f86f7; background:#2a3255; }";

static const char* kEditLt =
    "QLineEdit{"
    "  background:#ffffff; border:1px solid #cfd7ea; border-radius:6px;"
    "  color:#1e2340; padding:4px 8px;"
    "}"
    "QLineEdit:focus{ border-color:#4f86f7; background:#f8faff; }";

// Warning frame
static const char* kWarnFrameDk =
    "QFrame#warnFrame{"
    "  background:rgba(245,158,11,0.08);"
    "  border:1px solid rgba(245,158,11,0.25);"
    "  border-radius:7px;"
    "}";
static const char* kWarnFrameLt =
    "QFrame#warnFrame{"
    "  background:rgba(245,158,11,0.06);"
    "  border:1px solid rgba(245,158,11,0.3);"
    "  border-radius:7px;"
    "}";

// Content area background
static const char* kContentDk = "background:#141827;";
static const char* kContentLt = "background:#ffffff;";

// ─────────────────────────────────────────────────────────────────────────────
//  NavigableSpinBox
// ─────────────────────────────────────────────────────────────────────────────
NavigableSpinBox::NavigableSpinBox(QWidget* p) : QDoubleSpinBox(p)
{
    setGroupSeparatorShown(true);
    setRange(0, 1e12);
    setDecimals(2);
    setSingleStep(100);
    setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    updatePrefix();
}

void NavigableSpinBox::updatePrefix()
{
    setPrefix(currencyPrefix());
}

void NavigableSpinBox::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete) {
        double oldMin = minimum();
        setMinimum(-1e15);
        QDoubleSpinBox::keyPressEvent(e);
        if (value() < oldMin) setValue(oldMin);
        setMinimum(oldMin);
        return;
    }
    QDoubleSpinBox::keyPressEvent(e);
}

void NavigableSpinBox::focusInEvent(QFocusEvent* e)
{
    QDoubleSpinBox::focusInEvent(e);
    QTimer::singleShot(0, this, [this]() { lineEdit()->selectAll(); });
}

// ─────────────────────────────────────────────────────────────────────────────
//  MonthCard
// ─────────────────────────────────────────────────────────────────────────────
MonthCard::MonthCard(int monthIndex, QWidget* parent)
    : QFrame(parent), m_monthIndex(monthIndex)
{
    setObjectName("monthCard");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    buildHeader();
    root->addWidget(m_header);

    buildContent();
    root->addWidget(m_content);

    // Animation
    m_anim = new QPropertyAnimation(this, "contentHeight", this);
    m_anim->setDuration(220);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);

    // First month expanded by default
    if (m_monthIndex == 0) {
        m_content->setVisible(true);
        m_expanded = true;
        m_chevron->setText("▲");
        // Set actual height after layout
        QTimer::singleShot(0, this, [this]() {
            m_fullHeight = m_content->sizeHint().height();
            m_content->setMaximumHeight(m_fullHeight);
        });
    } else {
        m_content->setVisible(false);
        m_content->setMaximumHeight(0);
    }

    applyTheme();
    retranslate();
}

void MonthCard::buildHeader()
{
    m_header = new QWidget;
    m_header->setObjectName("cardHeader");
    m_header->setFixedHeight(52);
    m_header->setCursor(Qt::PointingHandCursor);
    m_header->setAttribute(Qt::WA_StyledBackground, true);

    auto* hl = new QHBoxLayout(m_header);
    hl->setContentsMargins(18, 0, 18, 0);
    hl->setSpacing(10);

    m_monthLabel = new QLabel;
    m_monthLabel->setStyleSheet(g_lightMode ? kMonthLblLt : kMonthLblDk);

    m_warnIcon = new QLabel("⚠");
    m_warnIcon->setStyleSheet("color:#f59e0b; background:transparent;");
    m_warnIcon->setVisible(false);
    m_warnIcon->setToolTip(T("Data warnings exist", "يوجد تحذيرات في البيانات"));

    hl->addWidget(m_monthLabel);
    hl->addWidget(m_warnIcon);
    hl->addStretch();

    m_chevron = new QLabel("▼");
    m_chevron->setStyleSheet(g_lightMode ? kChevronLt : kChevronDk);
    hl->addWidget(m_chevron);

    // Make header clickable
    m_header->installEventFilter(this);
}

void MonthCard::buildContent()
{
    m_content = new QWidget;
    m_content->setObjectName("cardContent");
    m_content->setAttribute(Qt::WA_StyledBackground, true);
    m_content->setStyleSheet(g_lightMode ? kContentLt : kContentDk);

    auto* vl = new QVBoxLayout(m_content);
    vl->setContentsMargins(16, 14, 16, 14);
    vl->setSpacing(12);

    // ── 4-column grid ─────────────────────────────────────────────────────
    auto* cols = new QHBoxLayout;
    cols->setSpacing(12);

    // Build all spinboxes
    m_sales         = makeSpin();
    m_salesReturn   = makeSpin(true);   // red tint
    m_suppPurchases = makeSpin();
    m_suppPayments  = makeSpin();
    m_invFirst      = makeSpin();
    m_invLast       = makeSpin();
    m_expAmount     = makeSpin();

    m_expAccount = new QLineEdit;
    m_expAccount->setPlaceholderText(T("e.g. Rent, Utilities", "مثال: إيجار، مرافق"));
    m_expAccount->setStyleSheet(g_lightMode ? kEditLt : kEditDk);

    // Connect all to updateWarnings
    auto connectSpin = [this](NavigableSpinBox* s) {
        connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &MonthCard::updateWarnings);
        connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &MonthCard::dataChanged);
    };
    connectSpin(m_sales); connectSpin(m_salesReturn);
    connectSpin(m_suppPurchases); connectSpin(m_suppPayments);
    connectSpin(m_invFirst); connectSpin(m_invLast);
    connectSpin(m_expAmount);
    connect(m_expAccount, &QLineEdit::textChanged, this, &MonthCard::dataChanged);

    // Column 1: Sales & Revenue
    auto* col1 = makeColumn(
        T("Sales & Revenue", "المبيعات والإيرادات"),
        {
            makeFieldRow(T("Sales Amount",  "مبلغ المبيعات"), m_sales),
            makeFieldRow(T("Sales Return",  "مردودات المبيعات"), m_salesReturn)
        }
    );
    // Column 2: Suppliers
    auto* col2 = makeColumn(
        T("Suppliers", "الموردون"),
        {
            makeFieldRow(T("Supplier Purchases", "مشتريات الموردين"), m_suppPurchases),
            makeFieldRow(T("Supplier Payments",  "مدفوعات الموردين"), m_suppPayments)
        }
    );
    // Column 3: Inventory
    auto* col3 = makeColumn(
        T("Inventory", "المخزون"),
        {
            makeFieldRow(T("Opening Stock (First Period)", "المخزون الافتتاحي"), m_invFirst),
            makeFieldRow(T("Closing Stock (Last Period)",  "المخزون الختامي"),  m_invLast)
        }
    );
    // Column 4: Expenses
    auto* col4 = makeColumn(
        T("Expenses", "المصروفات"),
        {
            makeFieldRow(T("Expense Account", "حساب المصروفات"), m_expAccount),
            makeFieldRow(T("Expense Amount",  "مبلغ المصروفات"),  m_expAmount)
        }
    );

    cols->addWidget(col1);
    cols->addWidget(col2);
    cols->addWidget(col3);
    cols->addWidget(col4);
    vl->addLayout(cols);

    // ── Warnings area ─────────────────────────────────────────────────────
    m_warnFrame = new QFrame;
    m_warnFrame->setObjectName("warnFrame");
    m_warnFrame->setAttribute(Qt::WA_StyledBackground, true);
    m_warnFrame->setStyleSheet(g_lightMode ? kWarnFrameLt : kWarnFrameDk);

    auto* wl = new QVBoxLayout(m_warnFrame);
    wl->setContentsMargins(12, 10, 12, 10);
    wl->setSpacing(4);

    auto* warnHdr = new QLabel("⚠  " + T("Warnings", "تحذيرات"));
    warnHdr->setStyleSheet("color:#f59e0b; font-weight:700; background:transparent;");
    wl->addWidget(warnHdr);

    m_warnList = new QLabel;
    m_warnList->setStyleSheet("color:#d97706; background:transparent;");
    m_warnList->setWordWrap(true);
    wl->addWidget(m_warnList);

    m_warnFrame->setVisible(false);
    vl->addWidget(m_warnFrame);
}

NavigableSpinBox* MonthCard::makeSpin(bool redTint)
{
    auto* s = new NavigableSpinBox;
    if (redTint)
        s->setStyleSheet(g_lightMode ? kSpinRedLt : kSpinRedDk);
    else
        s->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    return s;
}

QWidget* MonthCard::makeFieldRow(const QString& labelText, QWidget* input)
{
    auto* w = new QWidget;
    w->setAttribute(Qt::WA_StyledBackground, false);
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(3);

    auto* lbl = new QLabel(labelText);
    lbl->setObjectName("fieldLabel");
    lbl->setStyleSheet(g_lightMode ? kFieldLblLt : kFieldLblDk);

    vl->addWidget(lbl);
    vl->addWidget(input);
    return w;
}

QWidget* MonthCard::makeColumn(const QString& sectionTitle, QList<QWidget*> rows)
{
    auto* w = new QWidget;
    w->setAttribute(Qt::WA_StyledBackground, false);
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(10);

    auto* title = new QLabel(sectionTitle);
    title->setObjectName("sectionTitle");
    title->setStyleSheet(g_lightMode ? kSecTitleLt : kSecTitleDk);

    vl->addWidget(title);
    for (auto* row : rows)
        vl->addWidget(row);
    vl->addStretch();

    return w;
}

bool MonthCard::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_header && ev->type() == QEvent::MouseButtonRelease) {
        toggleExpand();
        return true;
    }
    return QFrame::eventFilter(obj, ev);
}

void MonthCard::toggleExpand()
{
    setExpanded(!m_expanded);
}

void MonthCard::setExpanded(bool expand)
{
    if (m_expanded == expand) return;
    m_expanded = expand;
    m_chevron->setText(m_expanded ? "▲" : "▼");

    if (m_expanded) {
        m_content->setVisible(true);
        m_fullHeight = m_content->sizeHint().height();
        if (m_fullHeight < 10) m_fullHeight = 250; // fallback
        m_anim->stop();
        m_anim->setStartValue(0);
        m_anim->setEndValue(m_fullHeight);
        m_anim->start();
    } else {
        m_fullHeight = m_content->maximumHeight();
        m_anim->stop();
        m_anim->setStartValue(m_fullHeight);
        m_anim->setEndValue(0);
        connect(m_anim, &QPropertyAnimation::finished, this, [this]() {
            if (!m_expanded) m_content->setVisible(false);
        }, Qt::SingleShotConnection);
        m_anim->start();
    }
}

int MonthCard::contentHeight() const
{
    return m_content->maximumHeight();
}

void MonthCard::setContentHeight(int h)
{
    m_content->setMaximumHeight(h);
    updateGeometry();
    if (parentWidget() && parentWidget()->parentWidget())
        parentWidget()->parentWidget()->update();
}

void MonthCard::updateWarnings()
{
    // Compute warnings same logic as the React app
    QStringList warnings;

    double inv1 = m_invFirst->value();
    double inv2 = m_invLast->value();
    double purch = m_suppPurchases->value();
    double sales = m_sales->value();
    double ret   = m_salesReturn->value();

    if (inv2 > inv1 + purch)
        warnings << T("Unusual stock increase: Closing inventory is greater than opening inventory + purchases.",
                      "زيادة غير معتادة في المخزون: المخزون الختامي أكبر من المخزون الافتتاحي + المشتريات.");

    double netSales = sales - ret;
    double cogs     = inv1 + purch - inv2;
    double profit   = netSales - cogs;
    if (profit < 0)
        warnings << T("Negative profit margin (Loss) detected.",
                      "تم رصد هامش ربح سلبي (خسارة).");

    bool hasWarnings = !warnings.isEmpty();
    m_warnIcon->setVisible(hasWarnings && !m_expanded);

    if (hasWarnings) {
        QString html;
        for (const QString& w : warnings)
            html += "• " + w + "<br>";
        m_warnList->setText(html.trimmed());
        m_warnFrame->setVisible(m_expanded);
    } else {
        m_warnFrame->setVisible(false);
    }

    // Recalculate full height when warnings change while expanded
    if (m_expanded) {
        QTimer::singleShot(10, this, [this]() {
            int newH = m_content->sizeHint().height();
            if (newH > 10) {
                m_fullHeight = newH;
                m_content->setMaximumHeight(newH);
            }
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

    // Section titles
    for (auto* lbl : m_content->findChildren<QLabel*>("sectionTitle"))
        lbl->setStyleSheet(g_lightMode ? kSecTitleLt : kSecTitleDk);
    // Field labels
    for (auto* lbl : m_content->findChildren<QLabel*>("fieldLabel"))
        lbl->setStyleSheet(g_lightMode ? kFieldLblLt : kFieldLblDk);

    // Spins
    if (m_sales)         m_sales->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    if (m_salesReturn)   m_salesReturn->setStyleSheet(g_lightMode ? kSpinRedLt : kSpinRedDk);
    if (m_suppPurchases) m_suppPurchases->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    if (m_suppPayments)  m_suppPayments->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    if (m_invFirst)      m_invFirst->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    if (m_invLast)       m_invLast->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    if (m_expAmount)     m_expAmount->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    if (m_expAccount)    m_expAccount->setStyleSheet(g_lightMode ? kEditLt : kEditDk);

    if (m_warnFrame) m_warnFrame->setStyleSheet(g_lightMode ? kWarnFrameLt : kWarnFrameDk);
}

void MonthCard::retranslate()
{
    const QStringList names = monthNames();
    m_monthLabel->setText(names.value(m_monthIndex));
    m_expAccount->setPlaceholderText(T("e.g. Rent, Utilities", "مثال: إيجار، مرافق"));
    m_warnIcon->setToolTip(T("Data warnings exist", "يوجد تحذيرات في البيانات"));

    // Re-label section titles and field labels
    // Section titles (by order they were created in columns)
    QList<QLabel*> secTitles = m_content->findChildren<QLabel*>("sectionTitle");
    if (secTitles.size() >= 4) {
        secTitles[0]->setText(T("Sales & Revenue", "المبيعات والإيرادات"));
        secTitles[1]->setText(T("Suppliers",        "الموردون"));
        secTitles[2]->setText(T("Inventory",        "المخزون"));
        secTitles[3]->setText(T("Expenses",         "المصروفات"));
    }
    QList<QLabel*> fieldLbls = m_content->findChildren<QLabel*>("fieldLabel");
    if (fieldLbls.size() >= 8) {
        fieldLbls[0]->setText(T("Sales Amount",               "مبلغ المبيعات"));
        fieldLbls[1]->setText(T("Sales Return",                "مردودات المبيعات"));
        fieldLbls[2]->setText(T("Supplier Purchases",          "مشتريات الموردين"));
        fieldLbls[3]->setText(T("Supplier Payments",           "مدفوعات الموردين"));
        fieldLbls[4]->setText(T("Opening Stock (First Period)","المخزون الافتتاحي"));
        fieldLbls[5]->setText(T("Closing Stock (Last Period)", "المخزون الختامي"));
        fieldLbls[6]->setText(T("Expense Account",             "حساب المصروفات"));
        fieldLbls[7]->setText(T("Expense Amount",              "مبلغ المصروفات"));
    }
}

void MonthCard::updateCurrencyPrefix()
{
    if (m_sales)         m_sales->updatePrefix();
    if (m_salesReturn)   m_salesReturn->updatePrefix();
    if (m_suppPurchases) m_suppPurchases->updatePrefix();
    if (m_suppPayments)  m_suppPayments->updatePrefix();
    if (m_invFirst)      m_invFirst->updatePrefix();
    if (m_invLast)       m_invLast->updatePrefix();
    if (m_expAmount)     m_expAmount->updatePrefix();
}

void MonthCard::clearAll()
{
    m_sales->setValue(0);
    m_salesReturn->setValue(0);
    m_suppPurchases->setValue(0);
    m_suppPayments->setValue(0);
    m_invFirst->setValue(0);
    m_invLast->setValue(0);
    m_expAmount->setValue(0);
    m_expAccount->clear();
    m_warnFrame->setVisible(false);
    m_warnIcon->setVisible(false);
}

// Getters
double  MonthCard::sales()             const { return m_sales->value(); }
double  MonthCard::salesReturn()       const { return m_salesReturn->value(); }
double  MonthCard::supplierPurchases() const { return m_suppPurchases->value(); }
double  MonthCard::supplierPayments()  const { return m_suppPayments->value(); }
QString MonthCard::expenseAccount()    const { return m_expAccount->text(); }
double  MonthCard::expenseAmount()     const { return m_expAmount->value(); }
double  MonthCard::inventoryFirst()    const { return m_invFirst->value(); }
double  MonthCard::inventoryLast()     const { return m_invLast->value(); }

// Setters
void MonthCard::setSales(double v)             { m_sales->setValue(v); }
void MonthCard::setSalesReturn(double v)       { m_salesReturn->setValue(v); }
void MonthCard::setSupplierPurchases(double v) { m_suppPurchases->setValue(v); }
void MonthCard::setSupplierPayments(double v)  { m_suppPayments->setValue(v); }
void MonthCard::setExpenseAccount(const QString& v) { m_expAccount->setText(v); }
void MonthCard::setExpenseAmount(double v)     { m_expAmount->setValue(v); }
void MonthCard::setInventoryFirst(double v)    { m_invFirst->setValue(v); }
void MonthCard::setInventoryLast(double v)     { m_invLast->setValue(v); }

// ─────────────────────────────────────────────────────────────────────────────
//  DataTableWidget
// ─────────────────────────────────────────────────────────────────────────────
DataTableWidget::DataTableWidget(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setStyleSheet(
        "QScrollArea{ background:transparent; border:none; }"
        "QScrollBar:vertical{ background:transparent; width:8px; border-radius:4px; }"
        "QScrollBar::handle:vertical{ background:#2e3860; border-radius:4px; min-height:30px; }"
        "QScrollBar::handle:vertical:hover{ background:#4f86f7; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical{ height:0; }");

    auto* container = new QWidget;
    container->setObjectName("cardsContainer");
    container->setAttribute(Qt::WA_StyledBackground, true);
    container->setStyleSheet("background:transparent;");

    auto* vl = new QVBoxLayout(container);
    vl->setContentsMargins(20, 16, 20, 24);
    vl->setSpacing(10);

    // Title bar
    auto* titleBar = new QWidget;
    titleBar->setStyleSheet("background:transparent;");
    auto* tl = new QVBoxLayout(titleBar);
    tl->setContentsMargins(0, 0, 0, 8);
    tl->setSpacing(2);

    auto* h2 = new QLabel(T("Data Entry", "إدخال البيانات"));
    h2->setStyleSheet(g_lightMode
        ? "color:#1e2340; font-weight:800; background:transparent;"
        : "color:#c8d0ed; font-weight:800; background:transparent;");
    auto* sub = new QLabel(T("Enter monthly figures below. Click a month to expand it.",
                              "أدخل الأرقام الشهرية أدناه. انقر على الشهر لتوسيعه."));
    sub->setStyleSheet("color:#5a6490; background:transparent;");

    tl->addWidget(h2);
    tl->addWidget(sub);
    vl->addWidget(titleBar);

    // 12 month cards
    for (int i = 0; i < 12; ++i) {
        m_cards[i] = new MonthCard(i, container);
        vl->addWidget(m_cards[i]);
    }

    vl->addStretch();
    m_scroll->setWidget(container);
    root->addWidget(m_scroll);
}

AppData DataTableWidget::collectData() const
{
    AppData d;
    for (int i = 0; i < 12; ++i) {
        auto& m = d.months[i];
        m.sales             = m_cards[i]->sales();
        m.salesReturn       = m_cards[i]->salesReturn();
        m.supplierPurchases = m_cards[i]->supplierPurchases();
        m.supplierPayments  = m_cards[i]->supplierPayments();
        m.expenseAccount    = m_cards[i]->expenseAccount();
        m.expenseAmount     = m_cards[i]->expenseAmount();
        m.inventoryFirst    = m_cards[i]->inventoryFirst();
        m.inventoryLast     = m_cards[i]->inventoryLast();
    }
    return d;
}

void DataTableWidget::setData(const AppData& d)
{
    for (int i = 0; i < 12; ++i) {
        const auto& m = d.months[i];
        m_cards[i]->setSales(m.sales);
        m_cards[i]->setSalesReturn(m.salesReturn);
        m_cards[i]->setSupplierPurchases(m.supplierPurchases);
        m_cards[i]->setSupplierPayments(m.supplierPayments);
        m_cards[i]->setExpenseAccount(m.expenseAccount);
        m_cards[i]->setExpenseAmount(m.expenseAmount);
        m_cards[i]->setInventoryFirst(m.inventoryFirst);
        m_cards[i]->setInventoryLast(m.inventoryLast);
        // Auto-expand months that have data
        if (i != 0) {
            bool hasData = m.sales || m.salesReturn || m.supplierPurchases ||
                           m.supplierPayments || m.expenseAmount ||
                           m.inventoryFirst || m.inventoryLast ||
                           !m.expenseAccount.isEmpty();
            if (hasData) m_cards[i]->setExpanded(true);
        }
    }
}

void DataTableWidget::clearData()
{
    for (int i = 0; i < 12; ++i)
        m_cards[i]->clearAll();
}

void DataTableWidget::updateCurrency()
{
    for (int i = 0; i < 12; ++i)
        m_cards[i]->updateCurrencyPrefix();
}

void DataTableWidget::retranslate()
{
    for (int i = 0; i < 12; ++i)
        m_cards[i]->retranslate();
}

void DataTableWidget::applyTheme()
{
    m_scroll->setStyleSheet(g_lightMode
        ? "QScrollArea{ background:transparent; border:none; }"
          "QScrollBar:vertical{ background:transparent; width:8px; border-radius:4px; }"
          "QScrollBar::handle:vertical{ background:#c8d0ed; border-radius:4px; min-height:30px; }"
          "QScrollBar::handle:vertical:hover{ background:#4f86f7; }"
          "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical{ height:0; }"
        : "QScrollArea{ background:transparent; border:none; }"
          "QScrollBar:vertical{ background:transparent; width:8px; border-radius:4px; }"
          "QScrollBar::handle:vertical{ background:#2e3860; border-radius:4px; min-height:30px; }"
          "QScrollBar::handle:vertical:hover{ background:#4f86f7; }"
          "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical{ height:0; }");

    for (int i = 0; i < 12; ++i)
        m_cards[i]->applyTheme();
}
