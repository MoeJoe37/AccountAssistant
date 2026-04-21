#include "Supplierswidget.h"
#include "translations.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QAbstractSpinBox>
#include <QLocale>
#include <QSizePolicy>
#include <QMouseEvent>

static const char* kCardDk = "QFrame#supplierCard{background:#141827;border:1px solid #252b4a;border-radius:10px;}";
static const char* kCardLt = "QFrame#supplierCard{background:#ffffff;border:1px solid #dde2f0;border-radius:10px;}";
static const char* kHdrDk = "QWidget#supplierHeader{background:#1a1f38;border-radius:9px 9px 0 0;}QWidget#supplierHeader:hover{background:#1e2445;}";
static const char* kHdrLt = "QWidget#supplierHeader{background:#f4f6fb;border-radius:9px 9px 0 0;}QWidget#supplierHeader:hover{background:#eef0fa;}";
static const char* kTitleDk = "color:#4f86f7;font-weight:800;background:transparent;";
static const char* kTitleLt = "color:#2563eb;font-weight:800;background:transparent;";
static const char* kLabelDk = "color:#5a6490;font-weight:600;background:transparent;";
static const char* kLabelLt = "color:#6b7280;font-weight:600;background:transparent;";
static const char* kMonthDk = "color:#c8d0ed;font-weight:700;background:transparent;";
static const char* kMonthLt = "color:#1e2340;font-weight:700;background:transparent;";
static const char* kChevronDk = "color:#5a6490;background:transparent;";
static const char* kChevronLt = "color:#8892b8;background:transparent;";
static const char* kSpinDk = "QDoubleSpinBox{background:#252d4a;border:1px solid #3a4268;border-radius:6px;color:#c8d0ed;font-family:'Consolas','Courier New',monospace;padding:4px 8px;text-align:right;}QDoubleSpinBox:focus{border-color:#4f86f7;}";
static const char* kSpinLt = "QDoubleSpinBox{background:#ffffff;border:1px solid #cfd7ea;border-radius:6px;color:#1e2340;font-family:'Consolas','Courier New',monospace;padding:4px 8px;text-align:right;}QDoubleSpinBox:focus{border-color:#4f86f7;}";
static const char* kRootDark = "QWidget#suppliersRoot{background:#0d1020;}QWidget#suppliersSubHeader{background:#111526;border-bottom:1px solid #1a1f38;}";
static const char* kRootLight = "QWidget#suppliersRoot{background:#f4f6fb;}QWidget#suppliersSubHeader{background:#ffffff;border-bottom:1px solid #dde2f0;}";

SupplierSpinBox::SupplierSpinBox(QWidget* parent) : QDoubleSpinBox(parent)
{
    setRange(0, 1e12);
    setDecimals(2);
    setSingleStep(100);
    setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    setGroupSeparatorShown(true);
    setButtonSymbols(QAbstractSpinBox::NoButtons);
}
void SupplierSpinBox::updatePrefix() { setPrefix(currencyPrefix()); }
void SupplierSpinBox::wheelEvent(QWheelEvent* e) { e->ignore(); }
void SupplierSpinBox::focusInEvent(QFocusEvent* e) { QDoubleSpinBox::focusInEvent(e); selectAll(); }

SupplierMonthCard::SupplierMonthCard(int monthIndex, QWidget* parent) : QFrame(parent), m_monthIndex(monthIndex)
{
    setObjectName("supplierCard");
    setAttribute(Qt::WA_StyledBackground, true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);
    buildHeader();
    buildContent();
    root->addWidget(m_header);
    root->addWidget(m_content);

    m_anim = new QPropertyAnimation(this, "contentHeight", this);
    m_anim->setDuration(220);
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
}

void SupplierMonthCard::buildHeader()
{
    m_header = new QWidget;
    m_header->setObjectName("supplierHeader");
    m_header->setFixedHeight(52);
    m_header->setCursor(Qt::PointingHandCursor);
    auto* hl = new QHBoxLayout(m_header);
    hl->setContentsMargins(18,0,18,0);
    m_monthLabel = new QLabel;
    m_chevron = new QLabel("▼");
    hl->addWidget(m_monthLabel);
    hl->addStretch();
    hl->addWidget(m_chevron);
    m_header->installEventFilter(this);
}

SupplierSpinBox* SupplierMonthCard::makeSpin()
{
    auto* s = new SupplierSpinBox;
    s->setMinimumWidth(220);
    s->updatePrefix();
    s->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    return s;
}

QWidget* SupplierMonthCard::makeFieldRow(const QString& labelText, QWidget* input)
{
    auto* w = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(3);
    auto* lbl = new QLabel(labelText);
    lbl->setObjectName("fieldLabel");
    lbl->setStyleSheet(g_lightMode ? kLabelLt : kLabelDk);
    vl->addWidget(lbl);
    vl->addWidget(input);
    return w;
}

QWidget* SupplierMonthCard::makeColumn(const QString& title, QList<QWidget*> rows)
{
    auto* w = new QWidget;
    auto* vl = new QVBoxLayout(w);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(10);
    auto* t = new QLabel(title);
    t->setObjectName("sectionTitle");
    t->setStyleSheet(g_lightMode ? kTitleLt : kTitleDk);
    vl->addWidget(t);
    for (auto* r : rows) vl->addWidget(r);
    vl->addStretch();
    return w;
}

void SupplierMonthCard::buildContent()
{
    m_content = new QWidget;
    m_content->setAttribute(Qt::WA_StyledBackground, true);
    auto* vl = new QVBoxLayout(m_content);
    vl->setContentsMargins(16,14,16,14);
    vl->setSpacing(12);

    m_nameEdit = new QLineEdit;
    m_nameEdit->setMinimumWidth(280);
    m_purchases = makeSpin();
    m_payments = makeSpin();

    connect(m_nameEdit, &QLineEdit::textChanged, this, &SupplierMonthCard::dataChanged);
    connect(m_purchases, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SupplierMonthCard::dataChanged);
    connect(m_payments, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SupplierMonthCard::dataChanged);

    auto* cols = new QHBoxLayout;
    cols->setSpacing(12);
    cols->addWidget(makeColumn(tr_suppliers_7beff3(), {
        makeFieldRow(tr_supplier_name_5c7e41(), m_nameEdit)
    }), 3);
    cols->addWidget(makeColumn(tr_suppliers_7beff3(), {
        makeFieldRow(tr_supplier_purchases_f5a1cd(), m_purchases),
        makeFieldRow(tr_supplier_payments_eeef31(), m_payments)
    }), 4);
    vl->addLayout(cols);
}

bool SupplierMonthCard::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_header && ev->type() == QEvent::MouseButtonRelease) {
        setExpanded(!m_expanded);
        return true;
    }
    return QFrame::eventFilter(obj, ev);
}

void SupplierMonthCard::setExpanded(bool e)
{
    if (m_expanded == e) return;
    m_expanded = e;
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

int SupplierMonthCard::contentHeight() const { return m_content ? m_content->maximumHeight() : 0; }
void SupplierMonthCard::setContentHeight(int h) { if (m_content) m_content->setMaximumHeight(h); }

QString SupplierMonthCard::supplierName() const { return m_nameEdit->text(); }
double SupplierMonthCard::purchases() const { return m_purchases->value(); }
double SupplierMonthCard::payments() const { return m_payments->value(); }
void SupplierMonthCard::setSupplierName(const QString& v) { m_nameEdit->setText(v); }
void SupplierMonthCard::setPurchases(double v) { m_purchases->setValue(v); }
void SupplierMonthCard::setPayments(double v) { m_payments->setValue(v); }
void SupplierMonthCard::clearAll() { m_nameEdit->clear(); m_purchases->setValue(0); m_payments->setValue(0); }

void SupplierMonthCard::updateCurrencyPrefix()
{
    m_purchases->updatePrefix();
    m_payments->updatePrefix();
}

void SupplierMonthCard::applyTheme()
{
    setStyleSheet(g_lightMode ? kCardLt : kCardDk);
    m_header->setStyleSheet(g_lightMode ? kHdrLt : kHdrDk);
    m_monthLabel->setStyleSheet(g_lightMode ? kMonthLt : kMonthDk);
    m_chevron->setStyleSheet(g_lightMode ? kChevronLt : kChevronDk);
    for (auto* lbl : m_content->findChildren<QLabel*>("sectionTitle")) lbl->setStyleSheet(g_lightMode ? kTitleLt : kTitleDk);
    for (auto* lbl : m_content->findChildren<QLabel*>("fieldLabel")) lbl->setStyleSheet(g_lightMode ? kLabelLt : kLabelDk);
    m_nameEdit->setStyleSheet(g_lightMode ? "background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:4px 8px;" : "background:#252d4a;color:#c8d0ed;border:1px solid #3a4268;border-radius:6px;padding:4px 8px;");
    m_purchases->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
    m_payments->setStyleSheet(g_lightMode ? kSpinLt : kSpinDk);
}

void SupplierMonthCard::retranslate()
{
    const QStringList names = monthNames();
    m_monthLabel->setText(names.value(m_monthIndex));
    updateCurrencyPrefix();

    const QList<QLabel*> titles = m_content->findChildren<QLabel*>("sectionTitle");
    if (titles.size() >= 2) {
        titles[0]->setText(tr_suppliers_7beff3());
        titles[1]->setText(tr_suppliers_7beff3());
    }

    const QList<QLabel*> fields = m_content->findChildren<QLabel*>("fieldLabel");
    if (fields.size() >= 3) {
        fields[0]->setText(tr_supplier_name_5c7e41());
        fields[1]->setText(tr_supplier_purchases_f5a1cd());
        fields[2]->setText(tr_supplier_payments_eeef31());
    }
}

SuppliersWidget::SuppliersWidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("suppliersRoot");
    setAttribute(Qt::WA_StyledBackground, true);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setStyleSheet("QScrollArea{background:transparent;border:none;}");

    auto* container = new QWidget;
    auto* vl = new QVBoxLayout(container);
    vl->setContentsMargins(20,16,20,24);
    vl->setSpacing(10);

    auto* titleBar = new QWidget;
    auto* tl = new QVBoxLayout(titleBar);
    tl->setContentsMargins(0,0,0,8);
    tl->setSpacing(2);
    m_title = new QLabel(tr_suppliers_7beff3());
    m_subtitle = new QLabel(tr_supplier_name_5c7e41());
    tl->addWidget(m_title);
    tl->addWidget(m_subtitle);
    vl->addWidget(titleBar);

    for (int i = 0; i < 12; ++i) {
        m_cards[i] = new SupplierMonthCard(i, container);
        vl->addWidget(m_cards[i]);
    }
    vl->addStretch();
    m_scroll->setWidget(container);
    updateCurrencyPrefix();
    root->addWidget(m_scroll);
    applyTheme();
    retranslate();
}

AppData SuppliersWidget::collectData() const
{
    AppData d;
    for (int i = 0; i < 12; ++i) {
        auto& m = d.suppliers[i];
        m.supplierName = m_cards[i]->supplierName();
        m.purchases = m_cards[i]->purchases();
        m.payments = m_cards[i]->payments();
    }
    return d;
}

void SuppliersWidget::setData(const AppData& data)
{
    for (int i = 0; i < 12; ++i) {
        const auto& m = data.suppliers[i];
        m_cards[i]->setSupplierName(m.supplierName);
        m_cards[i]->setPurchases(m.purchases);
        m_cards[i]->setPayments(m.payments);
        if (i != 0 && (!m.supplierName.isEmpty() || m.purchases || m.payments))
            m_cards[i]->setExpanded(true);
    }
}

void SuppliersWidget::clearData() { for (int i = 0; i < 12; ++i) m_cards[i]->clearAll(); }
void SuppliersWidget::updateCurrencyPrefix() { for (int i = 0; i < 12; ++i) m_cards[i]->updateCurrencyPrefix(); }
void SuppliersWidget::applyTheme() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(g_lightMode ? kRootLight : kRootDark);
    if (m_scroll && m_scroll->viewport()) {
        m_scroll->viewport()->setAutoFillBackground(true);
        m_scroll->viewport()->setStyleSheet(g_lightMode ? "background:#f4f6fb;" : "background:#0d1020;");
    }
    for (int i = 0; i < 12; ++i) m_cards[i]->applyTheme();
}
void SuppliersWidget::retranslate()
{
    if (m_title) m_title->setText(tr_suppliers_7beff3());
    if (m_subtitle) m_subtitle->setText(tr_supplier_name_5c7e41());
    for (int i = 0; i < 12; ++i)
        m_cards[i]->retranslate();
}
