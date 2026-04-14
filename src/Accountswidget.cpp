#include "Accountswidget.h"
#include "translations.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QSignalBlocker>
#include <QMessageBox>
#include <QWheelEvent>
#include <algorithm>

namespace {
class NoWheelDoubleSpinBox : public QDoubleSpinBox
{
public:
    using QDoubleSpinBox::QDoubleSpinBox;

protected:
    void wheelEvent(QWheelEvent* event) override
    {
        event->ignore();
    }
};

static const char* kAccountsSSDark = R"(
QWidget#accountsRoot { background:#0d1020; }
QWidget#accountsHeader { background:#111526; border-bottom:1px solid #1a1f38; }
QWidget#accountsContainer { background:#0d1020; }
QScrollArea#accountsScroll { background:#0d1020; border:none; }
QLabel#accountsTitle { color:#c8d0ed; font-weight:900; font-size:18px; background:transparent; }
QLabel#accountsSubtitle { color:#5a6490; background:transparent; }
QLabel#accountsGroupLabel { color:#c8d0ed; background:transparent; font-weight:700; }
QLineEdit, QDoubleSpinBox, QComboBox {
    background:#252d4a; color:#c8d0ed; border:1px solid #3a4268; border-radius:5px; padding:4px 8px;
}
QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus { border-color:#4f86f7; }
QPushButton#addAccountBtn, QToolButton#showGraphsBtn {
    background:#4f86f7; color:white; border:none; border-radius:7px; padding:8px 14px; font-weight:700;
}
QPushButton#addAccountBtn:hover, QToolButton#showGraphsBtn:hover { background:#5e91f8; }
QPushButton#addAccountBtn:pressed, QToolButton#showGraphsBtn:pressed { background:#3a6fe0; }
QPushButton#removeAccountBtn {
    background:#1a1f38; color:#e05c6a; border:1px solid #252b52; border-radius:6px; padding:6px 10px; font-weight:700;
}
QPushButton#removeAccountBtn:hover { background:#2c1530; }
QScrollArea { background:transparent; border:none; }
QScrollBar:vertical { background:#0d1020; width:8px; border-radius:4px; }
QScrollBar::handle:vertical { background:#2e3860; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#4f86f7; }
)";
static const char* kAccountsSSLight = R"(
QWidget#accountsRoot { background:#f4f6fb; }
QWidget#accountsHeader { background:#ffffff; border-bottom:1px solid #dde2f0; }
QWidget#accountsContainer { background:#f4f6fb; }
QScrollArea#accountsScroll { background:#f4f6fb; border:none; }
QLabel#accountsTitle { color:#1e2340; font-weight:900; font-size:18px; background:transparent; }
QLabel#accountsSubtitle { color:#6b7280; background:transparent; }
QLabel#accountsGroupLabel { color:#1e2340; background:transparent; font-weight:700; }
QLineEdit, QDoubleSpinBox, QComboBox {
    background:#ffffff; color:#1e2340; border:1px solid #cfd7ea; border-radius:5px; padding:4px 8px;
}
QLineEdit:focus, QDoubleSpinBox:focus, QComboBox:focus { border-color:#4f86f7; }
QPushButton#addAccountBtn, QToolButton#showGraphsBtn {
    background:#4f86f7; color:white; border:none; border-radius:7px; padding:8px 14px; font-weight:700;
}
QPushButton#addAccountBtn:hover, QToolButton#showGraphsBtn:hover { background:#5e91f8; }
QPushButton#addAccountBtn:pressed, QToolButton#showGraphsBtn:pressed { background:#3a6fe0; }
QPushButton#removeAccountBtn {
    background:#ffffff; color:#c0392b; border:1px solid #d9e0ef; border-radius:6px; padding:6px 10px; font-weight:700;
}
QPushButton#removeAccountBtn:hover { background:#fdf2f2; }
QScrollArea { background:transparent; border:none; }
QScrollBar:vertical { background:#f4f6fb; width:8px; border-radius:4px; }
QScrollBar::handle:vertical { background:#c8d0ed; border-radius:4px; min-height:30px; }
QScrollBar::handle:vertical:hover { background:#4f86f7; }
)";

static void showThemedMessageBox(QWidget* parent,
                                 QMessageBox::Icon icon,
                                 const QString& title,
                                 const QString& text,
                                 const QString& informative = QString())
{
    QMessageBox box(parent);
    box.setIcon(icon);
    box.setWindowTitle(title);
    box.setText(text);
    if (!informative.isEmpty())
        box.setInformativeText(informative);
    box.setTextFormat(Qt::PlainText);
    box.setStyleSheet(g_lightMode
        ? "QMessageBox{background:#f4f6fb; color:#1e2340;} QLabel{color:#1e2340;} QPushButton{background:#ffffff; color:#1e2340; border:1px solid #d9e0ef; border-radius:6px; padding:6px 14px; min-width:84px;} QPushButton:hover{background:#eef0fa;}"
        : "QMessageBox{background:#111526; color:#c8d0ed;} QLabel{color:#c8d0ed;} QPushButton{background:#1a1f38; color:#c8d0ed; border:1px solid #252b52; border-radius:6px; padding:6px 14px; min-width:84px;} QPushButton:hover{background:#1e2445;}");
    box.exec();
}
}

Accountswidget::Accountswidget(QWidget* parent) : QWidget(parent)
{
    setObjectName("accountsRoot");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    auto* header = new QWidget;
    header->setObjectName("accountsHeader");
    auto* hl = new QVBoxLayout(header);
    hl->setContentsMargins(20, 16, 20, 14);
    hl->setSpacing(8);

    m_title = new QLabel;
    m_title->setObjectName("accountsTitle");
    m_subtitle = new QLabel;
    m_subtitle->setObjectName("accountsSubtitle");
    m_subtitle->setWordWrap(true);
    hl->addWidget(m_title);
    hl->addWidget(m_subtitle);

    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(10);
    m_nameEdit = new QLineEdit;
    m_typeCombo = new QComboBox;
    m_typeCombo->addItem(accountTypeDisplayName(AccountType::Payable));
    m_typeCombo->addItem(accountTypeDisplayName(AccountType::Receivable));
    m_typeCombo->setCurrentIndex(0);
    m_typeCombo->setMinimumWidth(170);
    m_amountSpin = new NoWheelDoubleSpinBox;
    m_amountSpin->setRange(0, 1e12);
    m_amountSpin->setDecimals(2);
    m_amountSpin->setSingleStep(100);
    m_amountSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_addBtn = new QPushButton;
    m_addBtn->setObjectName("addAccountBtn");
    inputRow->addWidget(m_nameEdit, 2);
    inputRow->addWidget(m_typeCombo, 1);
    inputRow->addWidget(m_amountSpin, 1);
    inputRow->addWidget(m_addBtn);
    hl->addLayout(inputRow);

    auto* bottomRow = new QHBoxLayout;
    bottomRow->setSpacing(10);
    auto* sortLabel = new QLabel(T("Sort", "فرز"));
    sortLabel->setStyleSheet("background:transparent; font-weight:700;");
    auto* groupLabel = new QLabel(T("Group by", "تجميع حسب"));
    groupLabel->setObjectName("accountsGroupLabel");
    m_sortCombo = new QComboBox;
    m_sortCombo->addItem(T("Ascending", "تصاعدي"));
    m_sortCombo->addItem(T("Descending", "تنازلي"));
    m_groupCombo = new QComboBox;
    m_groupCombo->addItem(accountTypeFilterDisplayName(AccountTypeFilter::All));
    m_groupCombo->addItem(accountTypeFilterDisplayName(AccountTypeFilter::Payable));
    m_groupCombo->addItem(accountTypeFilterDisplayName(AccountTypeFilter::Receivable));
    m_graphBtn = new QToolButton;
    m_graphBtn->setObjectName("showGraphsBtn");
    bottomRow->addWidget(sortLabel);
    bottomRow->addWidget(m_sortCombo, 0, Qt::AlignLeft);
    bottomRow->addSpacing(12);
    bottomRow->addWidget(groupLabel);
    bottomRow->addWidget(m_groupCombo, 0, Qt::AlignLeft);
    bottomRow->addStretch();
    bottomRow->addWidget(m_graphBtn, 0, Qt::AlignRight);
    hl->addLayout(bottomRow);
    root->addWidget(header);

    m_scroll = new QScrollArea;
    m_scroll->setObjectName("accountsScroll");
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setStyleSheet("QScrollArea{background:transparent;border:none;}");

    m_container = new QWidget;
    m_container->setObjectName("accountsContainer");
    m_container->setAttribute(Qt::WA_StyledBackground, true);
    auto* vl = new QVBoxLayout(m_container);
    vl->setContentsMargins(20, 18, 20, 20);
    vl->setSpacing(10);
    m_rowsLayout = vl;

    m_empty = new QLabel;
    m_empty->setAlignment(Qt::AlignCenter);
    m_empty->setStyleSheet("background:transparent; color:#5a6490; font-weight:600;");
    vl->addWidget(m_empty);
    vl->addStretch();

    m_scroll->setWidget(m_container);
    root->addWidget(m_scroll, 1);

    connect(m_addBtn, &QPushButton::clicked, this, &Accountswidget::onAddAccount);
    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Accountswidget::onSortChanged);
    connect(m_groupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Accountswidget::onGroupChanged);
    connect(m_graphBtn, &QAbstractButton::clicked, this, &Accountswidget::onShowGraphs);

    retranslate();
    applyTheme();
    updateGraphButtonMenu();
}

AccountType Accountswidget::accountTypeFromIndex(int index)
{
    return index == 1 ? AccountType::Receivable : AccountType::Payable;
}

int Accountswidget::accountTypeIndex(AccountType type)
{
    return type == AccountType::Receivable ? 1 : 0;
}

AccountTypeFilter Accountswidget::currentGroupFilter() const
{
    if (!m_groupCombo)
        return AccountTypeFilter::All;
    switch (m_groupCombo->currentIndex()) {
    case 1: return AccountTypeFilter::Payable;
    case 2: return AccountTypeFilter::Receivable;
    default: return AccountTypeFilter::All;
    }
}

void Accountswidget::updatePrefixes()
{
    m_amountSpin->setPrefix(currencyPrefix());
    for (auto& row : m_rows) {
        if (row.amount) row.amount->setPrefix(currencyPrefix());
    }
}

void Accountswidget::applyTheme()
{
    setStyleSheet(g_lightMode ? kAccountsSSLight : kAccountsSSDark);
    if (m_scroll) {
        m_scroll->setStyleSheet(g_lightMode
            ? "QScrollArea#accountsScroll{background:#f4f6fb;border:none;}"
              " QScrollArea#accountsScroll QWidget#accountsContainer{background:#f4f6fb;}"
            : "QScrollArea#accountsScroll{background:#0d1020;border:none;}"
              " QScrollArea#accountsScroll QWidget#accountsContainer{background:#0d1020;}");
        if (m_scroll->viewport()) {
            m_scroll->viewport()->setAutoFillBackground(true);
        }
    }
    if (m_container) {
        m_container->setAttribute(Qt::WA_StyledBackground, true);
        m_container->setStyleSheet(g_lightMode ? "background:#f4f6fb;" : "background:#0d1020;");
    }
    updatePrefixes();
    applyGroupFilter();
}

void Accountswidget::retranslate()
{
    m_title->setText(T("Accounts", "الحسابات"));
    m_subtitle->setText(T(
        "Add expense accounts here. These accounts are independent from the monthly table.",
        "أضف حسابات المصروفات هنا. هذه الحسابات مستقلة عن جدول الأشهر."));
    m_nameEdit->setPlaceholderText(T("Expense account", "حساب المصروف"));
    m_amountSpin->setPrefix(currencyPrefix());
    m_addBtn->setText(T("+  Add", "+  إضافة"));
    m_sortCombo->setItemText(0, T("Ascending", "تصاعدي"));
    m_sortCombo->setItemText(1, T("Descending", "تنازلي"));
    if (m_typeCombo) {
        m_typeCombo->setItemText(0, accountTypeDisplayName(AccountType::Payable));
        m_typeCombo->setItemText(1, accountTypeDisplayName(AccountType::Receivable));
    }
    m_groupCombo->setItemText(0, accountTypeFilterDisplayName(AccountTypeFilter::All));
    m_groupCombo->setItemText(1, accountTypeFilterDisplayName(AccountTypeFilter::Payable));
    m_groupCombo->setItemText(2, accountTypeFilterDisplayName(AccountTypeFilter::Receivable));
    m_graphBtn->setText(T("Show graphs", "عرض الرسوم"));
    m_empty->setText(T("No accounts added yet.", "لم تتم إضافة حسابات بعد."));

    for (auto& row : m_rows) {
        if (!row.row) continue;
        if (row.name) row.name->setPlaceholderText(T("Expense account", "حساب المصروف"));
        if (row.type) {
            row.type->setItemText(0, accountTypeDisplayName(AccountType::Payable));
            row.type->setItemText(1, accountTypeDisplayName(AccountType::Receivable));
        }
        if (row.removeBtn) row.removeBtn->setText(T("Remove", "حذف"));
    }
    updateGraphButtonMenu();
}

void Accountswidget::updateGraphButtonMenu()
{
    if (m_graphBtn->menu()) {
        m_graphBtn->menu()->deleteLater();
    }
    auto* menu = new QMenu(m_graphBtn);
    QAction* pie = menu->addAction(T("Pie chart", "مخطط دائري"));
    pie->setData(int(ChartKind::Pie));
    QAction* bar = menu->addAction(T("Bar chart", "مخطط أعمدة"));
    bar->setData(int(ChartKind::RankedBar));
    QAction* line = menu->addAction(T("Line chart", "مخطط خطي"));
    line->setData(int(ChartKind::MetricLine));
    connect(menu, &QMenu::triggered, this, [this](QAction* act) {
        if (!act) return;
        emit graphRequested(static_cast<ChartKind>(act->data().toInt()), currentGroupFilter());
    });
    m_graphBtn->setMenu(menu);
    m_graphBtn->setPopupMode(QToolButton::InstantPopup);
}

bool Accountswidget::hasDuplicateName(const QString& name, const QLineEdit* except) const
{
    const QString key = name.trimmed().toCaseFolded();
    if (key.isEmpty())
        return false;

    for (const auto& row : m_rows) {
        if (!row.name || row.name == except)
            continue;
        if (row.name->text().trimmed().toCaseFolded() == key)
            return true;
    }
    return false;
}

void Accountswidget::addRow(const QString& name, double amount, AccountType type)
{
    if (m_empty)
        m_empty->hide();

    auto* rowW = new QWidget(m_container);
    rowW->setObjectName("accountRow");
    auto* hl = new QHBoxLayout(rowW);
    hl->setContentsMargins(0, 0, 0, 0);
    hl->setSpacing(10);

    auto* nameEdit = new QLineEdit(rowW);
    nameEdit->setPlaceholderText(T("Expense account", "حساب المصروف"));
    nameEdit->setText(name);

    auto* typeCombo = new QComboBox(rowW);
    typeCombo->addItem(accountTypeDisplayName(AccountType::Payable));
    typeCombo->addItem(accountTypeDisplayName(AccountType::Receivable));
    typeCombo->setCurrentIndex(accountTypeIndex(type));
    typeCombo->setMinimumWidth(170);

    auto* amountSpin = new NoWheelDoubleSpinBox(rowW);
    amountSpin->setRange(0, 1e12);
    amountSpin->setDecimals(2);
    amountSpin->setSingleStep(100);
    amountSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    amountSpin->setPrefix(currencyPrefix());
    amountSpin->setValue(amount);

    auto* removeBtn = new QPushButton(T("Remove", "حذف"), rowW);
    removeBtn->setObjectName("removeAccountBtn");
    removeBtn->setFixedWidth(80);

    hl->addWidget(nameEdit, 2);
    hl->addWidget(typeCombo, 1);
    hl->addWidget(amountSpin, 1);
    hl->addWidget(removeBtn);

    RowWidgets widgets;
    widgets.row = rowW;
    widgets.name = nameEdit;
    widgets.type = typeCombo;
    widgets.amount = amountSpin;
    widgets.removeBtn = removeBtn;
    widgets.lastValidName = name.trimmed();
    m_rows.append(widgets);

    connect(nameEdit, &QLineEdit::textChanged, this, &Accountswidget::onNameEdited);
    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Accountswidget::onRowChanged);
    connect(amountSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Accountswidget::onRowChanged);
    connect(removeBtn, &QPushButton::clicked, this, &Accountswidget::onRemoveRow);

    m_rowsLayout->insertWidget(qMax(0, m_rowsLayout->count() - 1), rowW);
    rowW->show();
    applyGroupFilter();
}

void Accountswidget::rebuildRows(const QList<AccountItem>& items)
{
    while (!m_rows.isEmpty()) {
        auto row = m_rows.takeLast();
        if (row.row) row.row->deleteLater();
    }
    if (m_empty) m_empty->show();

    QStringList seen;
    for (const auto& item : items) {
        const QString key = item.name.trimmed().toCaseFolded();
        if (!key.isEmpty() && seen.contains(key))
            continue;
        if (!key.isEmpty())
            seen.append(key);
        addRow(item.name, item.amount, item.type);
    }
    if (m_rows.isEmpty()) {
        if (m_empty) m_empty->show();
    }
    applyGroupFilter();
}

QList<AccountItem> Accountswidget::currentItems() const
{
    QList<AccountItem> items;
    QStringList seen;
    for (const auto& row : m_rows) {
        if (!row.name || !row.amount || !row.type) continue;
        const QString name = row.name->text().trimmed();
        const QString key = name.toCaseFolded();
        const double amount = row.amount->value();
        const AccountType type = accountTypeFromIndex(row.type->currentIndex());
        if (name.isEmpty() && qFuzzyIsNull(amount))
            continue;
        if (!key.isEmpty() && seen.contains(key))
            continue;
        if (!key.isEmpty())
            seen.append(key);
        items.append({name, amount, type});
    }
    return items;
}

void Accountswidget::sortAndRebuild()
{
    QList<AccountItem> items = currentItems();
    const bool asc = (m_sortCombo->currentIndex() == 0);
    std::sort(items.begin(), items.end(), [asc](const AccountItem& a, const AccountItem& b) {
        if (qFuzzyCompare(a.amount + 1.0, b.amount + 1.0))
            return asc ? (QString::localeAwareCompare(a.name, b.name) < 0) : (QString::localeAwareCompare(a.name, b.name) > 0);
        return asc ? (a.amount < b.amount) : (a.amount > b.amount);
    });
    rebuildRows(items);
    onRowChanged();
}

void Accountswidget::applyGroupFilter()
{
    const AccountTypeFilter filter = currentGroupFilter();
    int visibleCount = 0;
    for (auto& row : m_rows) {
        if (!row.row || !row.type)
            continue;
        const AccountType type = accountTypeFromIndex(row.type->currentIndex());
        bool visible = true;
        if (filter == AccountTypeFilter::Payable)
            visible = (type == AccountType::Payable);
        else if (filter == AccountTypeFilter::Receivable)
            visible = (type == AccountType::Receivable);
        row.row->setVisible(visible);
        if (visible)
            ++visibleCount;
    }

    if (!m_empty)
        return;

    if (m_rows.isEmpty()) {
        m_empty->setText(T("No accounts added yet.", "لم تتم إضافة حسابات بعد."));
        m_empty->show();
    } else if (visibleCount == 0) {
        m_empty->setText(T("No accounts match the selected group.", "لا توجد حسابات ضمن هذا التجميع."));
        m_empty->show();
    } else {
        m_empty->hide();
    }
}

void Accountswidget::onAddAccount()
{
    const QString name = m_nameEdit->text().trimmed();
    const double amount = m_amountSpin->value();
    const AccountType type = m_typeCombo ? accountTypeFromIndex(m_typeCombo->currentIndex()) : AccountType::Payable;
    if (name.isEmpty() && qFuzzyIsNull(amount))
        return;
    if (!name.isEmpty() && hasDuplicateName(name)) {
        showThemedMessageBox(this,
            QMessageBox::Warning,
            T("Duplicate account", "حساب مكرر"),
            T("An account with this name already exists.", "يوجد حساب بهذا الاسم بالفعل."));
        return;
    }
    addRow(name, amount, type);
    m_nameEdit->clear();
    if (m_typeCombo) m_typeCombo->setCurrentIndex(0);
    m_amountSpin->setValue(0.0);
    onRowChanged();
}

void Accountswidget::onSortChanged(int)
{
    sortAndRebuild();
}

void Accountswidget::onGroupChanged(int)
{
    applyGroupFilter();
    onRowChanged();
}

void Accountswidget::onShowGraphs()
{
    m_graphBtn->showMenu();
}

void Accountswidget::onNameEdited()
{
    auto* edit = qobject_cast<QLineEdit*>(sender());
    if (!edit) return;

    for (auto& row : m_rows) {
        if (row.name != edit)
            continue;

        const QString current = edit->text().trimmed();
        const QString currentKey = current.toCaseFolded();
        const QString lastKey = row.lastValidName.trimmed().toCaseFolded();

        if (!currentKey.isEmpty() && currentKey != lastKey && hasDuplicateName(current, edit)) {
            QSignalBlocker blocker(edit);
            edit->setText(row.lastValidName);
            edit->setCursorPosition(edit->text().length());
            showThemedMessageBox(this,
                QMessageBox::Warning,
                T("Duplicate account", "حساب مكرر"),
                T("An account with this name already exists.", "يوجد حساب بهذا الاسم بالفعل."));
            return;
        }

        row.lastValidName = current;
        break;
    }
    onRowChanged();
}

void Accountswidget::onRemoveRow()
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows[i].removeBtn == btn) {
            if (m_rows[i].row) m_rows[i].row->deleteLater();
            m_rows.removeAt(i);
            break;
        }
    }
    if (m_rows.isEmpty() && m_empty) m_empty->show();
    onRowChanged();
}

void Accountswidget::onRowChanged()
{
    applyGroupFilter();
}

AppData Accountswidget::collectData() const
{
    AppData d;
    d.accounts = currentItems();
    d.calculate();
    return d;
}

void Accountswidget::setData(const AppData& data)
{
    rebuildRows(data.accounts);
    if (m_rows.isEmpty() && m_empty) m_empty->show();
    onRowChanged();
}

void Accountswidget::clearData()
{
    rebuildRows({});
    if (m_empty) m_empty->show();
    onRowChanged();
}
