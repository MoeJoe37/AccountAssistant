#include "OtherRevenuesWidget.h"
#include "translations.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QGridLayout>
#include <QFrame>
#include <QSignalBlocker>
#include <QWheelEvent>
#include <QAbstractSpinBox>
#include <QAbstractButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QLocale>
#include <QMouseEvent>

namespace {
class NoWheelRevenueSpinBox : public QDoubleSpinBox
{
public:
    using QDoubleSpinBox::QDoubleSpinBox;
protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); }
};

static const char* kOtherRevenueDark = R"(
QWidget#otherRevenuesRoot { background:#0d1020; }
QWidget#otherRevenuesHeader { background:#111526; border-bottom:1px solid #1a1f38; }
QScrollArea#otherRevenuesScroll { background:#0d1020; border:none; }
QWidget#otherRevenuesContainer { background:#0d1020; }
QFrame#otherRevenueMonthCard { background:#141827; border:1px solid #252b4a; border-radius:10px; }
QWidget#otherRevenueMonthHeader { background:#1a1f38; border-radius:9px 9px 0 0; }
QWidget#otherRevenueMonthHeader:hover { background:#1e2445; }
QLabel#otherRevenueMonthLabel { color:#c8d0ed; font-weight:700; background:transparent; }
QLabel#otherRevenueChevron { color:#5a6490; background:transparent; }
QWidget#otherRevenueMonthContent { background:#141827; border-radius:0 0 9px 9px; }
QLabel#orTitle { color:#c8d0ed; font-weight:900; font-size:18px; background:transparent; }
QLabel#orSubtitle { color:#8a94bd; background:transparent; }
QLabel#orLabel { color:#c8d0ed; background:transparent; font-weight:800; }
QDoubleSpinBox { background:#252d4a; color:#c8d0ed; border:1px solid #3a4268; border-radius:6px; padding:6px 8px; }
QDoubleSpinBox:focus { border-color:#4f86f7; }
QScrollBar:vertical { background:#0d1020; width:8px; border-radius:4px; }
QScrollBar::handle:vertical { background:#2e3860; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#4f86f7; }
)";

static const char* kOtherRevenueLight = R"(
QWidget#otherRevenuesRoot { background:#f4f6fb; }
QWidget#otherRevenuesHeader { background:#ffffff; border-bottom:1px solid #dde2f0; }
QScrollArea#otherRevenuesScroll { background:#f4f6fb; border:none; }
QWidget#otherRevenuesContainer { background:#f4f6fb; }
QFrame#otherRevenueMonthCard { background:#ffffff; border:1px solid #dde2f0; border-radius:10px; }
QWidget#otherRevenueMonthHeader { background:#f4f6fb; border-radius:9px 9px 0 0; }
QWidget#otherRevenueMonthHeader:hover { background:#eef0fa; }
QLabel#otherRevenueMonthLabel { color:#1e2340; font-weight:700; background:transparent; }
QLabel#otherRevenueChevron { color:#8892b8; background:transparent; }
QWidget#otherRevenueMonthContent { background:#ffffff; border-radius:0 0 9px 9px; }
QLabel#orTitle { color:#1e2340; font-weight:900; font-size:18px; background:transparent; }
QLabel#orSubtitle { color:#6b7280; background:transparent; }
QLabel#orLabel { color:#1e2340; background:transparent; font-weight:800; }
QDoubleSpinBox { background:#ffffff; color:#1e2340; border:1px solid #cfd7ea; border-radius:6px; padding:6px 8px; }
QDoubleSpinBox:focus { border-color:#4f86f7; }
QScrollBar:vertical { background:#f4f6fb; width:8px; border-radius:4px; }
QScrollBar::handle:vertical { background:#c8d0ed; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#4f86f7; }
)";

static Qt::Alignment revenueVisualTextAlignment()
{
    return (isArabic() ? (Qt::AlignAbsolute | Qt::AlignRight)
                       : (Qt::AlignAbsolute | Qt::AlignLeft)) | Qt::AlignVCenter;
}

static Qt::Alignment revenueTextAlignment()
{
    return revenueVisualTextAlignment();
}

static Qt::Alignment revenueCaptionAlignment()
{
    return (isArabic() ? (Qt::AlignAbsolute | Qt::AlignRight)
                       : (Qt::AlignAbsolute | Qt::AlignLeft)) | Qt::AlignBottom;
}

static void applyRevenueLabelDirection(QLabel* label)
{
    if (!label)
        return;
    label->setLayoutDirection(appLayoutDirection());
    label->setAlignment(revenueCaptionAlignment());
    label->setContentsMargins(0, 0, 0, 0);
    label->setMinimumWidth(0);
}

static void applyRevenueSpinDirection(QDoubleSpinBox* spin)
{
    if (!spin)
        return;
    spin->setLayoutDirection(appLayoutDirection());
    spin->setAlignment(revenueVisualTextAlignment());
}

static QWidget* makeRevenueFieldBox(QWidget* parent, QLabel* caption, QWidget* field)
{
    auto* box = new QWidget(parent);
    box->setLayoutDirection(appLayoutDirection());
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    if (caption) {
        caption->setParent(box);
        caption->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        applyRevenueLabelDirection(caption);
        layout->addWidget(caption);
    }

    if (field) {
        field->setParent(box);
        field->setLayoutDirection(appLayoutDirection());
        field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        layout->addWidget(field);
    }

    return box;
}
}

OtherRevenuesWidget::OtherRevenuesWidget(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    retranslate();
    applyTheme();
    loadAllMonths();
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
    applyRevenueSpinDirection(s);
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
    headerLayout->setSpacing(8);

    m_title = new QLabel(header);
    m_title->setObjectName("orTitle");
    m_subtitle = new QLabel(header);
    m_subtitle->setObjectName("orSubtitle");
    m_subtitle->setWordWrap(true);
    headerLayout->addWidget(m_title);
    headerLayout->addWidget(m_subtitle);
    root->addWidget(header);

    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName("otherRevenuesScroll");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_container = new QWidget;
    m_container->setObjectName("otherRevenuesContainer");
    m_container->setAttribute(Qt::WA_StyledBackground, true);
    m_cardsLayout = new QVBoxLayout(m_container);
    m_cardsLayout->setContentsMargins(20, 16, 20, 24);
    m_cardsLayout->setSpacing(10);

    const QStringList months = monthNames();
    for (int i = 0; i < 12; ++i) {
        MonthWidgets card;
        card.monthIndex = i;
        card.expanded = (i == 0);
        card.card = new QFrame(m_container);
        card.card->setObjectName("otherRevenueMonthCard");
        card.card->setAttribute(Qt::WA_StyledBackground, true);

        auto* cardLayout = new QVBoxLayout(card.card);
        cardLayout->setContentsMargins(0, 0, 0, 0);
        cardLayout->setSpacing(0);

        card.header = new QWidget(card.card);
        card.header->setObjectName("otherRevenueMonthHeader");
        card.header->setAttribute(Qt::WA_StyledBackground, true);
        card.header->setCursor(Qt::PointingHandCursor);
        card.header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        card.header->setFixedHeight(52);
        card.header->setProperty("monthIndex", i);
        card.header->installEventFilter(this);

        auto* headerLayout = new QHBoxLayout(card.header);
        headerLayout->setContentsMargins(18, 0, 18, 0);
        headerLayout->setSpacing(10);

        card.monthLabel = new QLabel(card.header);
        card.monthLabel->setObjectName("otherRevenueMonthLabel");
        card.monthLabel->setAlignment(revenueTextAlignment());
        card.monthLabel->setLayoutDirection(appLayoutDirection());

        card.chevron = new QLabel(card.header);
        card.chevron->setObjectName("otherRevenueChevron");
        card.chevron->setAlignment(Qt::AlignCenter);

        headerLayout->addWidget(card.monthLabel);
        headerLayout->addStretch();
        headerLayout->addWidget(card.chevron);
        cardLayout->addWidget(card.header);

        card.content = new QWidget(card.card);
        card.content->setObjectName("otherRevenueMonthContent");
        card.content->setAttribute(Qt::WA_StyledBackground, true);
        card.content->setLayoutDirection(appLayoutDirection());
        auto* fieldsLayout = new QGridLayout(card.content);
        fieldsLayout->setContentsMargins(16, 14, 16, 16);
        fieldsLayout->setHorizontalSpacing(14);
        fieldsLayout->setVerticalSpacing(0);
        fieldsLayout->setOriginCorner(Qt::TopLeftCorner);

        card.privilegesLabel = new QLabel(card.content);
        card.privilegesLabel->setObjectName("orLabel");
        card.miscLabel = new QLabel(card.content);
        card.miscLabel->setObjectName("orLabel");
        card.privileges = makeSpin(card.content);
        card.misc = makeSpin(card.content);

        auto* privilegesField = makeRevenueFieldBox(card.content, card.privilegesLabel, card.privileges);
        auto* miscField = makeRevenueFieldBox(card.content, card.miscLabel, card.misc);
        const int privilegesCol = isArabic() ? 1 : 0;
        const int miscCol = isArabic() ? 0 : 1;
        fieldsLayout->addWidget(privilegesField, 0, privilegesCol);
        fieldsLayout->addWidget(miscField, 0, miscCol);
        fieldsLayout->setColumnStretch(privilegesCol, 1);
        fieldsLayout->setColumnStretch(miscCol, 1);

        cardLayout->addWidget(card.content);
        m_cardsLayout->addWidget(card.card);
        m_monthCards.append(card);

        connect(card.privileges, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            if (m_loading)
                return;
            syncAllMonths();
            emit dataChanged();
        });
        connect(card.misc, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            if (m_loading)
                return;
            syncAllMonths();
            emit dataChanged();
        });
        Q_UNUSED(months);
    }
    m_cardsLayout->addStretch();

    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll, 1);
}

void OtherRevenuesWidget::syncAllMonths() const
{
    for (const auto& card : m_monthCards) {
        const int month = card.monthIndex;
        if (month < 0 || month >= 12)
            continue;
        if (card.privileges)
            m_values[month].acquiredPrivileges = card.privileges->value();
        if (card.misc)
            m_values[month].miscellaneousRevenues = card.misc->value();
    }
}

void OtherRevenuesWidget::loadAllMonths()
{
    m_loading = true;
    for (auto& card : m_monthCards) {
        const int month = qBound(0, card.monthIndex, 11);
        if (card.privileges) {
            QSignalBlocker b(card.privileges);
            card.privileges->setValue(m_values[month].acquiredPrivileges);
        }
        if (card.misc) {
            QSignalBlocker b(card.misc);
            card.misc->setValue(m_values[month].miscellaneousRevenues);
        }
        updateMonthCardText(card);
        if (card.content)
            card.content->setVisible(card.expanded);
    }
    m_loading = false;
}

void OtherRevenuesWidget::updateMonthCardText(MonthWidgets& card)
{
    const QStringList months = monthNames();
    const QString monthName = (card.monthIndex >= 0 && card.monthIndex < months.size()) ? months.value(card.monthIndex) : QString();
    const QString arrow = card.expanded ? QStringLiteral("▲") : QStringLiteral("▼");
    if (card.header) {
        card.header->setLayoutDirection(Qt::LeftToRight);
        if (auto* layout = dynamic_cast<QHBoxLayout*>(card.header->layout())) {
            while (QLayoutItem* item = layout->takeAt(0))
                delete item;
            layout->setDirection(QBoxLayout::LeftToRight);
            if (isArabic()) {
                if (card.chevron)
                    layout->addWidget(card.chevron);
                layout->addStretch();
                if (card.monthLabel)
                    layout->addWidget(card.monthLabel);
            } else {
                if (card.monthLabel)
                    layout->addWidget(card.monthLabel);
                layout->addStretch();
                if (card.chevron)
                    layout->addWidget(card.chevron);
            }
        }
    }
    if (card.monthLabel) {
        card.monthLabel->setText(monthName);
        card.monthLabel->setLayoutDirection(appLayoutDirection());
        card.monthLabel->setAlignment(isArabic() ? (Qt::AlignRight | Qt::AlignVCenter) : (Qt::AlignLeft | Qt::AlignVCenter));
    }
    if (card.chevron)
        card.chevron->setText(arrow);
    if (card.content) {
        card.content->setLayoutDirection(appLayoutDirection());
        if (auto* fields = dynamic_cast<QGridLayout*>(card.content->layout())) {
            fields->setOriginCorner(Qt::TopLeftCorner);
            // Reposition the two field blocks explicitly so Arabic does not depend on
            // QBoxLayout mirroring. This keeps each label directly above its own field.
            QWidget* privilegesBox = card.privilegesLabel ? card.privilegesLabel->parentWidget() : nullptr;
            QWidget* miscBox = card.miscLabel ? card.miscLabel->parentWidget() : nullptr;
            if (privilegesBox && miscBox) {
                fields->removeWidget(privilegesBox);
                fields->removeWidget(miscBox);
                const int privilegesCol = isArabic() ? 1 : 0;
                const int miscCol = isArabic() ? 0 : 1;
                fields->addWidget(privilegesBox, 0, privilegesCol);
                fields->addWidget(miscBox, 0, miscCol);
                fields->setColumnStretch(privilegesCol, 1);
                fields->setColumnStretch(miscCol, 1);
                privilegesBox->setLayoutDirection(appLayoutDirection());
                miscBox->setLayoutDirection(appLayoutDirection());
            }
        }
    }
    if (card.privilegesLabel) {
        card.privilegesLabel->setText(tr_acquired_privileges_6a72d2());
        applyRevenueLabelDirection(card.privilegesLabel);
    }
    if (card.miscLabel) {
        card.miscLabel->setText(tr_other_misc_revenues_a330db());
        applyRevenueLabelDirection(card.miscLabel);
    }
    applyRevenueSpinDirection(card.privileges);
    applyRevenueSpinDirection(card.misc);
}


bool OtherRevenuesWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event && event->type() == QEvent::MouseButtonRelease) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton) {
            for (int i = 0; i < m_monthCards.size(); ++i) {
                if (obj == m_monthCards[i].header) {
                    setMonthExpanded(i, !m_monthCards[i].expanded);
                    return true;
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void OtherRevenuesWidget::setMonthExpanded(int month, bool expanded)
{
    if (month < 0 || month >= m_monthCards.size())
        return;
    auto& card = m_monthCards[month];
    card.expanded = expanded;
    if (card.content)
        card.content->setVisible(expanded);
    updateMonthCardText(card);
}

AppData OtherRevenuesWidget::collectData() const
{
    syncAllMonths();
    AppData d;
    d.otherRevenues = m_values;
    d.calculate();
    return d;
}

void OtherRevenuesWidget::setData(const AppData& data)
{
    m_values = data.otherRevenues;
    loadAllMonths();
}

void OtherRevenuesWidget::clearData()
{
    m_values = {};
    loadAllMonths();
    emit dataChanged();
}

void OtherRevenuesWidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kOtherRevenueLight : kOtherRevenueDark);
    if (m_container)
        m_container->setStyleSheet(g_lightMode ? QStringLiteral("background:#f4f6fb;") : QStringLiteral("background:#0d1020;"));
}

void OtherRevenuesWidget::retranslate()
{
    syncAllMonths();
    setLayoutDirection(appLayoutDirection());
    if (m_title) {
        m_title->setText(tr_other_revenues_title_19ce34());
        m_title->setAlignment(revenueTextAlignment());
    }
    if (m_subtitle) {
        m_subtitle->setText(tr_other_revenues_subtitle_4f60a1());
        m_subtitle->setAlignment(revenueTextAlignment());
    }
    for (auto& card : m_monthCards)
        updateMonthCardText(card);
}

void OtherRevenuesWidget::updateCurrencyPrefix()
{
    for (auto& card : m_monthCards) {
        for (QDoubleSpinBox* spin : {card.privileges, card.misc}) {
            if (!spin)
                continue;
            spin->setPrefix(currencyPrefix());
            spin->setSuffix(currencySuffix());
            spin->setDecimals(currencyDecimals());
            spin->setSingleStep(g_currency == AppCurrency::IQD ? 1000.0 : 100.0);
            applyRevenueSpinDirection(spin);
        }
    }
}
