#include "OtherRevenuesWidget.h"
#include "translations.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QSignalBlocker>
#include <QWheelEvent>
#include <QAbstractSpinBox>
#include <QLocale>

namespace {
class NoWheelRevenueSpinBox : public QDoubleSpinBox
{
public:
    using QDoubleSpinBox::QDoubleSpinBox;
protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); }
};

class NoWheelRevenueComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;
protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); }
};

static const char* kOtherRevenueDark = R"(
QWidget#otherRevenuesRoot { background:#0d1020; }
QWidget#otherRevenuesHeader { background:#111526; border-bottom:1px solid #1a1f38; }
QFrame#otherRevenueCard { background:#111526; border:1px solid #252b52; border-radius:12px; }
QLabel#orTitle { color:#c8d0ed; font-weight:900; font-size:18px; background:transparent; }
QLabel#orSubtitle { color:#8a94bd; background:transparent; }
QLabel#orLabel { color:#c8d0ed; background:transparent; font-weight:800; }
QComboBox, QDoubleSpinBox { background:#252d4a; color:#c8d0ed; border:1px solid #3a4268; border-radius:6px; padding:6px 8px; }
QComboBox:focus, QDoubleSpinBox:focus { border-color:#4f86f7; }
QComboBox QAbstractItemView { background:#1a1f38; color:#c8d0ed; selection-background-color:#4f86f7; }
)";

static const char* kOtherRevenueLight = R"(
QWidget#otherRevenuesRoot { background:#f4f6fb; }
QWidget#otherRevenuesHeader { background:#ffffff; border-bottom:1px solid #dde2f0; }
QFrame#otherRevenueCard { background:#ffffff; border:1px solid #dde2f0; border-radius:12px; }
QLabel#orTitle { color:#1e2340; font-weight:900; font-size:18px; background:transparent; }
QLabel#orSubtitle { color:#6b7280; background:transparent; }
QLabel#orLabel { color:#1e2340; background:transparent; font-weight:800; }
QComboBox, QDoubleSpinBox { background:#ffffff; color:#1e2340; border:1px solid #cfd7ea; border-radius:6px; padding:6px 8px; }
QComboBox:focus, QDoubleSpinBox:focus { border-color:#4f86f7; }
QComboBox QAbstractItemView { background:#ffffff; color:#1e2340; selection-background-color:#eef0fa; }
)";
}

OtherRevenuesWidget::OtherRevenuesWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    retranslate();
    applyTheme();
    loadCurrentMonth();
}

QDoubleSpinBox* OtherRevenuesWidget::makeSpin(QWidget* parent)
{
    auto* s = new NoWheelRevenueSpinBox(parent);
    s->setRange(-999999999999.99, 999999999999.99);
    s->setDecimals(currencyDecimals());
    s->setSingleStep(g_currency == AppCurrency::IQD ? 1000.0 : 100.0);
    s->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
    s->setGroupSeparatorShown(true);
    s->setButtonSymbols(QAbstractSpinBox::NoButtons);
    s->setKeyboardTracking(false);
    s->setPrefix(currencyPrefix());
    s->setSuffix(currencySuffix());
    s->setAlignment(appTextAlign() | Qt::AlignVCenter);
    return s;
}

void OtherRevenuesWidget::buildUi()
{
    setObjectName("otherRevenuesRoot");
    setLayoutDirection(appLayoutDirection());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QWidget(this);
    header->setObjectName("otherRevenuesHeader");
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(20, 16, 20, 14);
    headerLayout->setSpacing(10);

    m_title = new QLabel(header);
    m_title->setObjectName("orTitle");
    m_subtitle = new QLabel(header);
    m_subtitle->setObjectName("orSubtitle");
    m_subtitle->setWordWrap(true);
    headerLayout->addWidget(m_title);
    headerLayout->addWidget(m_subtitle);

    auto* selectorRow = new QHBoxLayout;
    selectorRow->setDirection(isArabic() ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight);
    selectorRow->setSpacing(10);
    m_monthLabel = new QLabel(header);
    m_monthLabel->setObjectName("orLabel");
    m_monthCombo = new NoWheelRevenueComboBox(header);
    m_monthCombo->setMinimumWidth(190);
    const QStringList months = monthNames();
    for (int i = 0; i < 12; ++i)
        m_monthCombo->addItem(months.value(i), i);
    selectorRow->addWidget(m_monthLabel);
    selectorRow->addWidget(m_monthCombo);
    selectorRow->addStretch();
    headerLayout->addLayout(selectorRow);
    root->addWidget(header);

    auto* body = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(20, 20, 20, 20);
    bodyLayout->setSpacing(12);

    auto* card = new QFrame(body);
    card->setObjectName("otherRevenueCard");
    auto* grid = new QGridLayout(card);
    grid->setContentsMargins(18, 16, 18, 16);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(8);
    grid->setOriginCorner(isArabic() ? Qt::TopRightCorner : Qt::TopLeftCorner);

    m_privilegesLabel = new QLabel(card);
    m_privilegesLabel->setObjectName("orLabel");
    m_privilegesLabel->setAlignment(appTextAlign() | Qt::AlignVCenter);
    m_miscLabel = new QLabel(card);
    m_miscLabel->setObjectName("orLabel");
    m_miscLabel->setAlignment(appTextAlign() | Qt::AlignVCenter);
    m_privileges = makeSpin(card);
    m_misc = makeSpin(card);

    grid->addWidget(m_privilegesLabel, 0, 0);
    grid->addWidget(m_miscLabel, 0, 1);
    grid->addWidget(m_privileges, 1, 0);
    grid->addWidget(m_misc, 1, 1);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

    bodyLayout->addWidget(card);
    bodyLayout->addStretch();
    root->addWidget(body, 1);

    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_loading)
            return;
        syncCurrentMonth();
        m_currentMonth = m_monthCombo ? m_monthCombo->currentData().toInt() : 0;
        loadCurrentMonth();
    });

    const auto spinChanged = [this](double) {
        if (m_loading)
            return;
        syncCurrentMonth();
        emit dataChanged();
    };
    connect(m_privileges, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, spinChanged);
    connect(m_misc, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, spinChanged);
}

void OtherRevenuesWidget::syncCurrentMonth() const
{
    if (m_currentMonth < 0 || m_currentMonth >= 12)
        return;
    if (m_privileges)
        m_values[m_currentMonth].acquiredPrivileges = m_privileges->value();
    if (m_misc)
        m_values[m_currentMonth].miscellaneousRevenues = m_misc->value();
}

void OtherRevenuesWidget::loadCurrentMonth()
{
    m_loading = true;
    const int month = qBound(0, m_currentMonth, 11);
    if (m_privileges) {
        QSignalBlocker b(m_privileges);
        m_privileges->setValue(m_values[month].acquiredPrivileges);
    }
    if (m_misc) {
        QSignalBlocker b(m_misc);
        m_misc->setValue(m_values[month].miscellaneousRevenues);
    }
    m_loading = false;
}

AppData OtherRevenuesWidget::collectData() const
{
    syncCurrentMonth();
    AppData d;
    d.otherRevenues = m_values;
    d.calculate();
    return d;
}

void OtherRevenuesWidget::setData(const AppData& data)
{
    m_values = data.otherRevenues;
    m_currentMonth = qBound(0, m_currentMonth, 11);
    if (m_monthCombo) {
        const QSignalBlocker blocker(m_monthCombo);
        const int idx = m_monthCombo->findData(m_currentMonth);
        if (idx >= 0)
            m_monthCombo->setCurrentIndex(idx);
    }
    loadCurrentMonth();
}

void OtherRevenuesWidget::clearData()
{
    m_values = {};
    loadCurrentMonth();
    emit dataChanged();
}

void OtherRevenuesWidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kOtherRevenueLight : kOtherRevenueDark);
}

void OtherRevenuesWidget::retranslate()
{
    syncCurrentMonth();
    setLayoutDirection(appLayoutDirection());
    if (m_title) m_title->setText(tr_other_revenues_title_19ce34());
    if (m_subtitle) m_subtitle->setText(tr_other_revenues_subtitle_4f60a1());
    if (m_monthLabel) m_monthLabel->setText(tr_expense_months_dropdown_label_62ac11());
    if (m_privilegesLabel) {
        m_privilegesLabel->setText(tr_acquired_privileges_6a72d2());
        m_privilegesLabel->setAlignment(appTextAlign() | Qt::AlignVCenter);
    }
    if (m_miscLabel) {
        m_miscLabel->setText(tr_other_misc_revenues_a330db());
        m_miscLabel->setAlignment(appTextAlign() | Qt::AlignVCenter);
    }
    if (m_monthCombo) {
        const QSignalBlocker blocker(m_monthCombo);
        const int selected = m_monthCombo->currentData().isValid() ? m_monthCombo->currentData().toInt() : m_currentMonth;
        m_monthCombo->clear();
        const QStringList months = monthNames();
        for (int i = 0; i < 12; ++i)
            m_monthCombo->addItem(months.value(i), i);
        const int idx = m_monthCombo->findData(qBound(0, selected, 11));
        if (idx >= 0)
            m_monthCombo->setCurrentIndex(idx);
    }
    if (auto* grid = findChild<QGridLayout*>())
        grid->setOriginCorner(isArabic() ? Qt::TopRightCorner : Qt::TopLeftCorner);
    updateCurrencyPrefix();
    loadCurrentMonth();
}

void OtherRevenuesWidget::updateCurrencyPrefix()
{
    for (auto* spin : {m_privileges, m_misc}) {
        if (!spin)
            continue;
        spin->setDecimals(currencyDecimals());
        spin->setSingleStep(g_currency == AppCurrency::IQD ? 1000.0 : 100.0);
        spin->setPrefix(currencyPrefix());
        spin->setSuffix(currencySuffix());
        spin->setAlignment(appTextAlign() | Qt::AlignVCenter);
    }
}
