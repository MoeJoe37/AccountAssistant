#include "Supplierswidget.h"
#include "translations.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QAbstractSpinBox>
#include <QLocale>
#include <QWheelEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QEasingCurve>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QMenu>

static const char* kRootDark = "QWidget#suppliersRoot{background:#0d1020;} QLabel{background:transparent;}";
static const char* kRootLight = "QWidget#suppliersRoot{background:#f4f6fb;} QLabel{background:transparent;}";
static const char* kCardDark = "QFrame#supplierCard{background:#141827;border:1px solid #252b4a;border-radius:10px;}";
static const char* kCardLight = "QFrame#supplierCard{background:#ffffff;border:1px solid #dde2f0;border-radius:10px;}";
static const char* kHdrDark = "QWidget#supplierHeader{background:#1a1f38;border-radius:9px 9px 0 0;}QWidget#supplierHeader:hover{background:#1e2445;}";
static const char* kHdrLight = "QWidget#supplierHeader{background:#f4f6fb;border-radius:9px 9px 0 0;}QWidget#supplierHeader:hover{background:#eef0fa;}";
static const char* kContentDark = "QWidget#supplierContent{background:#141827;border-radius:0 0 9px 9px;}";
static const char* kContentLight = "QWidget#supplierContent{background:#ffffff;border-radius:0 0 9px 9px;}";
static const char* kMonthLblDark = "color:#c8d0ed;font-weight:800;background:transparent;";
static const char* kMonthLblLight = "color:#1e2340;font-weight:800;background:transparent;";
static const char* kChevronDark = "color:#8892b8;background:transparent;font-weight:800;";
static const char* kChevronLight = "color:#6b7280;background:transparent;font-weight:800;";

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
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_header = new QWidget;
    m_header->setObjectName("supplierHeader");
    m_header->setFixedHeight(52);
    m_header->setCursor(Qt::PointingHandCursor);
    m_header->setAttribute(Qt::WA_StyledBackground, true);
    auto* top = new QHBoxLayout(m_header);
    top->setContentsMargins(16, 0, 16, 0);
    top->setSpacing(10);
    m_monthLabel = new QLabel;
    m_chevron = new QLabel("▼");
    m_addBtn = new QPushButton;
    m_addBtn->setCursor(Qt::PointingHandCursor);
    m_addBtn->setFixedHeight(30);
    top->addWidget(m_monthLabel);
    top->addStretch();
    top->addWidget(m_addBtn);
    top->addWidget(m_chevron);
    m_header->installEventFilter(this);
    root->addWidget(m_header);

    m_content = new QWidget;
    m_content->setObjectName("supplierContent");
    m_content->setAttribute(Qt::WA_StyledBackground, true);
    auto* contentLayout = new QVBoxLayout(m_content);
    contentLayout->setContentsMargins(12, 12, 12, 12);
    contentLayout->setSpacing(8);

    m_labelsRow = new QWidget;
    m_labelsLayout = new QGridLayout(m_labelsRow);
    m_labelsLayout->setContentsMargins(0,0,0,0);
    m_labelsLayout->setHorizontalSpacing(6);
    m_labelsLayout->setVerticalSpacing(2);
    m_labelsRow->setVisible(false);

    m_rowsLayout = new QVBoxLayout;
    m_rowsLayout->setContentsMargins(0,0,0,0);
    m_rowsLayout->setSpacing(6);
    contentLayout->addLayout(m_rowsLayout);

    root->addWidget(m_content);

    m_anim = new QPropertyAnimation(this, "contentHeight", this);
    m_anim->setDuration(220);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_addBtn, &QPushButton::clicked, this, &SupplierMonthCard::addSupplierRequested);
    setRowCount(1);

    if (m_monthIndex == 0) {
        m_content->setVisible(true);
        m_expanded = true;
        m_chevron->setText("▲");
        QTimer::singleShot(0, this, [this]{ m_fullHeight = qMax(10, m_content->sizeHint().height()); m_content->setMaximumHeight(m_fullHeight); });
    } else {
        m_content->setVisible(false);
        m_content->setMaximumHeight(0);
    }

    retranslate();
    applyTheme();
}

SupplierSpinBox* SupplierMonthCard::makeSpin(bool readOnly)
{
    auto* s = new SupplierSpinBox;
    s->setReadOnly(readOnly);
    s->setButtonSymbols(QAbstractSpinBox::NoButtons);
    s->setFocusPolicy(readOnly ? Qt::NoFocus : Qt::StrongFocus);
    if (readOnly)
        s->setSpecialValueText(QString());
    return s;
}

QLabel* SupplierMonthCard::makeResultLabel()
{
    auto* l = new QLabel;
    l->setAlignment(Qt::AlignCenter);
    l->setMinimumWidth(145);
    l->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    l->setFixedHeight(32);
    return l;
}

void SupplierMonthCard::appendRow()
{
    RowWidgets rw;
    rw.row = new QWidget;
    auto* grid = new QGridLayout(rw.row);
    grid->setContentsMargins(0,0,0,0);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(3);

    auto mkFieldLabel = [this](const QString& text) {
        auto* lbl = new QLabel(text);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setWordWrap(false);
        lbl->setProperty("supplierFieldLabel", true);
        lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        lbl->setFixedHeight(24);
        return lbl;
    };

    rw.nameLabel = mkFieldLabel(T("Supplier name", "اسم المورد"));
    rw.previousBalanceLabel = mkFieldLabel(T("Previous balance", "الرصيد السابق"));
    rw.purchasesLabel = mkFieldLabel(T("Purchases", "المشتريات"));
    rw.totalDebtLabel = mkFieldLabel(T("Total debt", "إجمالي الدين"));
    rw.paymentsLabel = mkFieldLabel(T("Payments", "الدفعات"));
    rw.pctPurchasesLabel = mkFieldLabel(T("Payment % of purchases", "نسبة الدفع من المشتريات"));
    rw.pctDebtLabel = mkFieldLabel(T("Payment % of debt", "نسبة الدفع من الدين"));
    rw.balanceLabel = mkFieldLabel(T("Supplier balance", "رصيد المورد"));

    rw.name = new QLineEdit;
    rw.name->setMinimumWidth(180);
    rw.name->setFixedHeight(32);
    rw.name->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rw.previousBalance = makeSpin(m_monthIndex != 0);
    rw.previousBalance->setMinimumWidth(135);
    rw.previousBalance->setFixedHeight(32);
    rw.purchases = makeSpin(false);
    rw.purchases->setMinimumWidth(135);
    rw.purchases->setFixedHeight(32);
    rw.totalDebt = makeSpin(true);
    rw.totalDebt->setMinimumWidth(135);
    rw.totalDebt->setFixedHeight(32);
    rw.payments = makeSpin(false);
    rw.payments->setMinimumWidth(135);
    rw.payments->setFixedHeight(32);
    rw.pctPurchases = makeResultLabel();
    rw.pctDebt = makeResultLabel();
    rw.balance = makeResultLabel();

    const int lr = 0;
    const int wr = 1;
    grid->addWidget(rw.nameLabel, lr, 0);
    grid->addWidget(rw.previousBalanceLabel, lr, 1);
    grid->addWidget(rw.purchasesLabel, lr, 2);
    grid->addWidget(rw.totalDebtLabel, lr, 3);
    grid->addWidget(rw.paymentsLabel, lr, 4);
    grid->addWidget(rw.pctPurchasesLabel, lr, 5);
    grid->addWidget(rw.pctDebtLabel, lr, 6);
    grid->addWidget(rw.balanceLabel, lr, 7);
    grid->addWidget(rw.name, wr, 0);
    grid->addWidget(rw.previousBalance, wr, 1);
    grid->addWidget(rw.purchases, wr, 2);
    grid->addWidget(rw.totalDebt, wr, 3);
    grid->addWidget(rw.payments, wr, 4);
    grid->addWidget(rw.pctPurchases, wr, 5);
    grid->addWidget(rw.pctDebt, wr, 6);
    grid->addWidget(rw.balance, wr, 7);
    for (int c = 0; c < 8; ++c)
        grid->setColumnStretch(c, c == 0 ? 2 : 1);

    const int idx = m_rows.size();
    connect(rw.name, &QLineEdit::textChanged, this, [this, idx](const QString& text){ emit supplierNameEdited(idx, text); emit monthChanged(); });
    auto bindChange = [this](QDoubleSpinBox* s) {
        connect(s, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double){ emit monthChanged(); });
    };
    bindChange(rw.previousBalance);
    bindChange(rw.purchases);
    bindChange(rw.payments);

    m_rowsLayout->addWidget(rw.row);
    m_rows.push_back(rw);
    setupRowContextMenu(m_rows.back(), idx);
    updateStyles();
}


void SupplierMonthCard::setupRowContextMenu(RowWidgets& rw, int rowIndex)
{
    auto bindMenu = [this, rowIndex](QWidget* w) {
        if (!w) return;
        w->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(w, &QWidget::customContextMenuRequested, this, [this, rowIndex, w](const QPoint& pos) {
            if (m_rows.size() <= 1) return;
            QMenu menu;
            const QString removeText = T("Remove supplier", "حذف المورد");
            QAction* act = menu.addAction(removeText);
            if (menu.exec(w->mapToGlobal(pos)) == act)
                emit removeSupplierRequested(rowIndex);
        });
    };
    bindMenu(rw.row);
    bindMenu(rw.name);
    bindMenu(rw.previousBalance);
    bindMenu(rw.purchases);
    bindMenu(rw.totalDebt);
    bindMenu(rw.payments);
    bindMenu(rw.pctPurchases);
    bindMenu(rw.pctDebt);
    bindMenu(rw.balance);
}

bool SupplierMonthCard::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj == m_header && ev->type() == QEvent::MouseButtonRelease) {
        toggleExpand();
        return true;
    }
    return QFrame::eventFilter(obj, ev);
}

void SupplierMonthCard::toggleExpand()
{
    setExpanded(!m_expanded);
}

void SupplierMonthCard::setAddButtonVisible(bool visible)
{
    if (!m_addBtn) return;
    m_addBtn->setVisible(visible);
    m_addBtn->setEnabled(visible);
}

void SupplierMonthCard::setExpanded(bool expand)
{
    if (m_expanded == expand) return;
    m_expanded = expand;
    if (m_chevron) m_chevron->setText(m_expanded ? "▲" : "▼");
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

int SupplierMonthCard::contentHeight() const
{
    return m_content ? m_content->maximumHeight() : 0;
}

void SupplierMonthCard::setContentHeight(int h)
{
    if (m_content) m_content->setMaximumHeight(h);
}

void SupplierMonthCard::setRowCount(int count)
{
    while (m_rows.size() < count)
        appendRow();
    while (m_rows.size() > count) {
        auto rw = m_rows.takeLast();
        delete rw.row;
    }
    QTimer::singleShot(0, this, [this]{
        m_fullHeight = qMax(10, m_content->sizeHint().height());
        if (m_expanded) m_content->setMaximumHeight(m_fullHeight);
    });
}

int SupplierMonthCard::rowCount() const { return m_rows.size(); }

QList<SupplierEntry> SupplierMonthCard::entries() const
{
    QList<SupplierEntry> out;
    for (const auto& r : m_rows) {
        SupplierEntry e;
        e.name = r.name->text().trimmed();
        e.previousBalance = r.previousBalance->value();
        e.purchases = r.purchases->value();
        e.totalDebt = r.totalDebt->value();
        e.payments = r.payments->value();
        out.append(e);
    }
    return out;
}

void SupplierMonthCard::setEntries(const QList<SupplierEntry>& entries)
{
    setRowCount(qMax(1, entries.size()));
    for (int i = 0; i < m_rows.size(); ++i) {
        const SupplierEntry e = i < entries.size() ? entries[i] : SupplierEntry{};
        auto& r = m_rows[i];
        r.name->setText(e.name);
        r.previousBalance->setValue(e.previousBalance);
        r.purchases->setValue(e.purchases);
        r.totalDebt->setValue(e.totalDebt > 0.0 ? e.totalDebt : (e.previousBalance + e.purchases));
        r.payments->setValue(e.payments);
    }
}

void SupplierMonthCard::refreshComputedValues(const QList<SupplierEntry>* previousMonthEntries)
{
    for (int i = 0; i < m_rows.size(); ++i) {
        auto& r = m_rows[i];
        if (previousMonthEntries && i < previousMonthEntries->size()) {
            const double carry = previousMonthEntries->at(i).supplierBalance();
            if (m_monthIndex != 0)
                r.previousBalance->setValue(carry);
        }
        const double debt = r.previousBalance->value() + r.purchases->value();
        r.totalDebt->setValue(debt);
        const double pctPurch = r.purchases->value() > 0.0 ? (r.payments->value() / r.purchases->value()) * 100.0 : 0.0;
        const double pctDebt = debt > 0.0 ? (r.payments->value() / debt) * 100.0 : 0.0;
        const double bal = debt - r.payments->value();
        r.pctPurchases->setText(QString::number(pctPurch, 'f', 2) + "%");
        r.pctDebt->setText(QString::number(pctDebt, 'f', 2) + "%");
        r.balance->setText(currencyPrefix() + QLocale(QLocale::English, QLocale::UnitedStates).toString(bal, 'f', 2));
    }
    if (m_expanded) {
        QTimer::singleShot(0, this, [this]{
            m_fullHeight = qMax(10, m_content->sizeHint().height());
            m_content->setMaximumHeight(m_fullHeight);
        });
    }
}

void SupplierMonthCard::setNamesFrom(const QStringList& names)
{
    setRowCount(names.size());
    for (int i = 0; i < names.size() && i < m_rows.size(); ++i) {
        if (m_rows[i].name->text().trimmed() != names[i].trimmed()) {
            QSignalBlocker blocker(m_rows[i].name);
            m_rows[i].name->setText(names[i].trimmed());
        }
    }
}

void SupplierMonthCard::clearAll()
{
    for (auto& r : m_rows) {
        r.name->clear();
        r.previousBalance->setValue(0.0);
        r.purchases->setValue(0.0);
        r.totalDebt->setValue(0.0);
        r.payments->setValue(0.0);
        r.pctPurchases->clear();
        r.pctDebt->clear();
        r.balance->clear();
    }
}

void SupplierMonthCard::updateStyles()
{
    setStyleSheet(g_lightMode ? kCardLight : kCardDark);
    if (m_header) m_header->setStyleSheet(g_lightMode ? kHdrLight : kHdrDark);
    if (m_content) m_content->setStyleSheet(g_lightMode ? kContentLight : kContentDark);
    const QString lineEdit = g_lightMode
        ? "QLineEdit{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:4px 8px;}"
        : "QLineEdit{background:#252d4a;color:#c8d0ed;border:1px solid #3a4268;border-radius:6px;padding:4px 8px;}";
    const QString spin = g_lightMode
        ? "QDoubleSpinBox{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:4px 8px;}"
        : "QDoubleSpinBox{background:#252d4a;color:#c8d0ed;border:1px solid #3a4268;border-radius:6px;padding:4px 8px;}";
    const QString roSpin = g_lightMode
        ? "QDoubleSpinBox{background:#f4f6fb;color:#1e2340;border:1px solid #d8deec;border-radius:6px;padding:4px 8px;}"
        : "QDoubleSpinBox{background:#1a1f38;color:#c8d0ed;border:1px solid #2b3258;border-radius:6px;padding:4px 8px;}";
    const QString resultSS = g_lightMode
        ? "QLabel{background:#eef2ff;color:#1e2340;border:1px solid #d8deec;border-radius:6px;padding:6px;font-weight:700;}"
        : "QLabel{background:#1a1f38;color:#c8d0ed;border:1px solid #2b3258;border-radius:6px;padding:6px;font-weight:700;}";
    const QString btn = g_lightMode
        ? "QPushButton{background:#4f86f7;color:white;border:none;border-radius:6px;padding:0 12px;font-weight:700;}"
        : "QPushButton{background:#4f86f7;color:white;border:none;border-radius:6px;padding:0 12px;font-weight:700;}";
    if (m_addBtn) m_addBtn->setStyleSheet(btn);
    if (m_monthLabel) m_monthLabel->setStyleSheet(g_lightMode ? kMonthLblLight : kMonthLblDark);
    if (m_chevron) m_chevron->setStyleSheet(g_lightMode ? kChevronLight : kChevronDark);
    for (auto& r : m_rows) {
        r.name->setStyleSheet(lineEdit);
        r.previousBalance->setStyleSheet(r.previousBalance->isReadOnly() ? roSpin : spin);
        r.purchases->setStyleSheet(spin);
        r.totalDebt->setStyleSheet(roSpin);
        r.payments->setStyleSheet(spin);
        const QString fieldLbl = g_lightMode
        ? "QLabel{background:#e9eefb;color:#1e2340;border:1px solid #d8deec;border-radius:6px;padding:2px 4px;font-weight:800;}"
        : "QLabel{background:#202745;color:#c8d0ed;border:1px solid #2b3258;border-radius:6px;padding:2px 4px;font-weight:800;}";
        for (QLabel* lbl : {r.nameLabel, r.previousBalanceLabel, r.purchasesLabel, r.totalDebtLabel, r.paymentsLabel, r.pctPurchasesLabel, r.pctDebtLabel, r.balanceLabel}) {
            if (lbl) lbl->setStyleSheet(fieldLbl);
        }
        r.pctPurchases->setStyleSheet(resultSS);
        r.pctDebt->setStyleSheet(resultSS);
        r.balance->setStyleSheet(resultSS);
    }
}

void SupplierMonthCard::applyTheme() { updateStyles(); }
void SupplierMonthCard::updateCurrencyPrefix()
{
    for (auto& r : m_rows) {
        r.previousBalance->updatePrefix();
        r.purchases->updatePrefix();
        r.totalDebt->updatePrefix();
        r.payments->updatePrefix();
    }
}

void SupplierMonthCard::retranslate()
{
    const QStringList months = monthNames();
    if (m_monthLabel) m_monthLabel->setText(months.value(m_monthIndex));
    if (m_addBtn) m_addBtn->setText(T("+ Add supplier", "+ إضافة مورد"));

    for (auto& r : m_rows) {
        if (r.nameLabel) r.nameLabel->setText(T("Supplier name", "اسم المورد"));
        if (r.previousBalanceLabel) r.previousBalanceLabel->setText(T("Previous balance", "الرصيد السابق"));
        if (r.purchasesLabel) r.purchasesLabel->setText(T("Purchases", "المشتريات"));
        if (r.totalDebtLabel) r.totalDebtLabel->setText(T("Total debt", "إجمالي الدين"));
        if (r.paymentsLabel) r.paymentsLabel->setText(T("Payments", "الدفعات"));
        if (r.pctPurchasesLabel) r.pctPurchasesLabel->setText(T("Payment % of purchases", "نسبة الدفع من المشتريات"));
        if (r.pctDebtLabel) r.pctDebtLabel->setText(T("Payment % of debt", "نسبة الدفع من الدين"));
        if (r.balanceLabel) r.balanceLabel->setText(T("Supplier balance", "رصيد المورد"));

        r.name->setPlaceholderText(T("Supplier name", "اسم المورد"));
        r.previousBalance->setToolTip(T("Previous balance", "الرصيد السابق"));
        r.purchases->setToolTip(T("Purchases", "المشتريات"));
        r.totalDebt->setToolTip(T("Total debt", "إجمالي الدين"));
        r.payments->setToolTip(T("Payments", "الدفعات"));
        r.pctPurchases->setToolTip(T("Payment % of monthly purchases", "نسبة الدفع من مشتريات الشهر"));
        r.pctDebt->setToolTip(T("Payment % of total debt", "نسبة الدفع من إجمالي الدين"));
        r.balance->setToolTip(T("Supplier balance", "رصيد المورد"));
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
    auto* container = new QWidget;
    auto* vl = new QVBoxLayout(container);
    vl->setContentsMargins(20,16,20,24);
    vl->setSpacing(10);

    m_title = new QLabel;
    m_subtitle = new QLabel;
    m_subtitle->setWordWrap(true);
    vl->addWidget(m_title);
    vl->addWidget(m_subtitle);

    for (int i = 0; i < 12; ++i) {
        m_cards[i] = new SupplierMonthCard(i, container);
        m_cards[i]->setAddButtonVisible(i == 0);
        connect(m_cards[i], &SupplierMonthCard::addSupplierRequested, this, &SuppliersWidget::onAddSupplierRequested);
        connect(m_cards[i], &SupplierMonthCard::supplierNameEdited, this, &SuppliersWidget::onSupplierNameEdited);
        connect(m_cards[i], &SupplierMonthCard::monthChanged, this, &SuppliersWidget::onMonthChanged);
        connect(m_cards[i], &SupplierMonthCard::removeSupplierRequested, this, &SuppliersWidget::onRemoveSupplierRequested);
        vl->addWidget(m_cards[i]);
    }
    vl->addStretch();
    m_scroll->setWidget(container);
    root->addWidget(m_scroll);
    ensureGlobalSupplierCount(1);
    retranslate();
    applyTheme();
    refreshAllComputedValues();
}

void SuppliersWidget::ensureGlobalSupplierCount(int count)
{
    count = qMax(1, count);
    for (int i = 0; i < 12; ++i)
        m_cards[i]->setRowCount(count);
}

QStringList SuppliersWidget::currentSupplierNames() const
{
    QStringList names;
    const auto entries = m_cards[0]->entries();
    for (const auto& e : entries)
        names << e.name;
    return names;
}

void SuppliersWidget::refreshAllComputedValues()
{
    for (int i = 0; i < 12; ++i) {
        if (i == 0) m_cards[i]->refreshComputedValues(nullptr);
        else {
            const auto prev = m_cards[i-1]->entries();
            m_cards[i]->refreshComputedValues(&prev);
        }
    }
}

void SuppliersWidget::onAddSupplierRequested()
{
    ensureGlobalSupplierCount(m_cards[0]->rowCount() + 1);
    refreshAllComputedValues();
}

void SuppliersWidget::onSupplierNameEdited(int rowIndex, const QString& name)
{
    Q_UNUSED(rowIndex);
    Q_UNUSED(name);
    QStringList names = currentSupplierNames();
    for (int i = 1; i < 12; ++i)
        m_cards[i]->setNamesFrom(names);
    refreshAllComputedValues();
}

void SuppliersWidget::onMonthChanged()
{
    refreshAllComputedValues();
}


void SuppliersWidget::onRemoveSupplierRequested(int rowIndex)
{
    const int currentCount = m_cards[0]->rowCount();
    if (currentCount <= 1 || rowIndex < 0 || rowIndex >= currentCount)
        return;

    for (int i = 0; i < 12; ++i) {
        auto entries = m_cards[i]->entries();
        if (rowIndex >= 0 && rowIndex < entries.size())
            entries.removeAt(rowIndex);
        if (entries.isEmpty())
            entries.append(SupplierEntry{});
        m_cards[i]->setEntries(entries);
    }
    ensureGlobalSupplierCount(currentCount - 1);
    refreshAllComputedValues();
}

AppData SuppliersWidget::collectData() const
{
    AppData d;
    for (int i = 0; i < 12; ++i) {
        d.supplierEntries[i] = m_cards[i]->entries();
        double purch = 0.0, pay = 0.0;
        QString first;
        for (const auto& e : d.supplierEntries[i]) {
            purch += e.purchases;
            pay += e.payments;
            if (first.isEmpty() && !e.name.trimmed().isEmpty()) first = e.name.trimmed();
        }
        d.suppliers[i].supplierName = first;
        d.suppliers[i].purchases = purch;
        d.suppliers[i].payments = pay;
    }
    return d;
}

void SuppliersWidget::setData(const AppData& data)
{
    bool hasAnySupplierEntries = false;
    int maxRows = 1;
    for (int i = 0; i < 12; ++i) {
        maxRows = qMax(maxRows, data.supplierEntries[i].size());
        if (!data.supplierEntries[i].isEmpty())
            hasAnySupplierEntries = true;
    }
    ensureGlobalSupplierCount(maxRows);
    if (hasAnySupplierEntries) {
        for (int i = 0; i < 12; ++i)
            m_cards[i]->setEntries(data.supplierEntries[i]);
    } else {
        for (int i = 0; i < 12; ++i) {
            QList<SupplierEntry> entries;
            SupplierEntry e;
            e.name = data.suppliers[i].supplierName;
            e.purchases = data.suppliers[i].purchases;
            e.payments = data.suppliers[i].payments;
            entries << e;
            m_cards[i]->setEntries(entries);
        }
    }
    refreshAllComputedValues();
}

void SuppliersWidget::clearData()
{
    ensureGlobalSupplierCount(1);
    for (int i = 0; i < 12; ++i)
        m_cards[i]->clearAll();
    refreshAllComputedValues();
}

void SuppliersWidget::updateCurrencyPrefix()
{
    for (int i = 0; i < 12; ++i)
        m_cards[i]->updateCurrencyPrefix();
    refreshAllComputedValues();
}

void SuppliersWidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kRootLight : kRootDark);
    if (m_scroll && m_scroll->viewport())
        m_scroll->viewport()->setStyleSheet(g_lightMode ? "background:#f4f6fb;" : "background:#0d1020;");
    if (m_title) m_title->setStyleSheet(g_lightMode ? "font-size:18px;font-weight:900;color:#1e2340;" : "font-size:18px;font-weight:900;color:#c8d0ed;");
    if (m_subtitle) m_subtitle->setStyleSheet(g_lightMode ? "color:#6b7280;" : "color:#8892b8;");
    for (int i = 0; i < 12; ++i)
        m_cards[i]->applyTheme();
}

void SuppliersWidget::retranslate()
{
    if (m_title) m_title->setText(T("Suppliers", "الموردون"));
    if (m_subtitle) m_subtitle->setText(T("Each row tracks one supplier across all months. Previous balance rolls forward automatically.", "كل صف يتابع مورداً واحداً عبر جميع الأشهر. يتم ترحيل الرصيد السابق تلقائياً."));
    for (int i = 0; i < 12; ++i)
        m_cards[i]->retranslate();
}
