#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>

static const char* kSetSSDark = R"(
QDialog { background:#12152a; }
QLabel#title { color:#4f86f7; font-weight:800; }
QGroupBox {
    color:#5a6490; font-weight:700; border:1px solid #252b52; border-radius:8px; margin-top:12px; padding:12px;
}
QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 6px; color:#4f86f7; }
QRadioButton { color:#c8d0ed; spacing:10px; }
QRadioButton::indicator {
    width:18px; height:18px;
    border:2px solid #3a4470; border-radius:9px; background:#252d4a;
}
QRadioButton::indicator:checked { background:#4f86f7; border-color:#4f86f7; }
QCheckBox { color:#c8d0ed; spacing:10px; }
QCheckBox::indicator {
    width:18px; height:18px;
    border:2px solid #3a4470; border-radius:5px; background:#252d4a;
}
QCheckBox::indicator:checked { background:#4f86f7; border-color:#4f86f7; }
QCheckBox::indicator:checked:hover { background:#6a9df9; }
QPushButton {
    background:#4f86f7; color:white; font-weight:700;
    border:none; border-radius:7px; min-height:38px; padding:0 24px;
}
QPushButton:hover { background:#6a9df9; }
QPushButton#cancel { background:#1e2340; color:#8892b8; border:1px solid #2e3455; }
QPushButton#cancel:hover { background:#252b50; }
QComboBox {
    background:#252d4a; color:#c8d0ed; border:1px solid #3a4268;
    border-radius:5px; padding:4px 8px; }
QComboBox::drop-down { border:none; }
QComboBox QAbstractItemView {
    background:#1e2340; color:#c8d0ed; border:1px solid #3a4470;
    selection-background-color:#2e3660;
}
)";

static const char* kSetSSLt = R"(
QDialog { background:#f4f6fb; }
QLabel#title { color:#1e2340; font-weight:800; }
QGroupBox {
    color:#5a6490; font-weight:700; border:1px solid #dde2f0; border-radius:8px; margin-top:12px; padding:12px; background:#ffffff;
}
QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 6px; color:#4f86f7; }
QRadioButton { color:#1e2340; spacing:10px; }
QRadioButton::indicator {
    width:18px; height:18px;
    border:2px solid #cfd7ea; border-radius:9px; background:#ffffff;
}
QRadioButton::indicator:checked { background:#4f86f7; border-color:#4f86f7; }
QCheckBox { color:#1e2340; spacing:10px; }
QCheckBox::indicator {
    width:18px; height:18px;
    border:2px solid #cfd7ea; border-radius:5px; background:#ffffff;
}
QCheckBox::indicator:checked { background:#4f86f7; border-color:#4f86f7; }
QCheckBox::indicator:checked:hover { background:#6a9df9; }
QPushButton {
    background:#4f86f7; color:white; font-weight:700;
    border:none; border-radius:7px; min-height:38px; padding:0 24px;
}
QPushButton:hover { background:#6a9df9; }
QPushButton#cancel { background:#eef0fa; color:#5a6490; border:1px solid #dde2f0; }
QPushButton#cancel:hover { background:#e6e9f6; }
QComboBox {
    background:#ffffff; color:#1e2340; border:1px solid #cfd7ea;
    border-radius:5px; padding:4px 8px; }
QComboBox::drop-down { border:none; }
QComboBox QAbstractItemView {
    background:#ffffff; color:#1e2340; border:1px solid #dde2f0;
    selection-background-color:#eef0fa;
}
)";

SettingsDialog::SettingsDialog(AppLanguage current, bool lightMode,
                               AppCurrency currency, int fontSize,
                               bool classicView,
                               QWidget* parent)
    : QDialog(parent)
{
    setStyleSheet(lightMode ? kSetSSLt : kSetSSDark);
    setWindowTitle(T("Settings","\u0627\u0644\u0625\u0639\u062f\u0627\u062f\u0627\u062a"));
    setMinimumWidth(400);

    auto* vl = new QVBoxLayout(this);
    vl->setContentsMargins(28,24,28,24);
    vl->setSpacing(14);

    auto* title = new QLabel(T("Settings","\u0625\u0639\u062f\u0627\u062f\u0627\u062a \u0627\u0644\u062a\u0637\u0628\u064a\u0642"));
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    vl->addWidget(title);

    // ── Language group ────────────────────────────────────────────────────
    auto* langGrp = new QGroupBox(T("Language","\u0627\u0644\u0644\u063a\u0629"));
    auto* gl  = new QVBoxLayout(langGrp);
    gl->setSpacing(12);
    m_en = new QRadioButton("English");
    m_ar = new QRadioButton("\u0639\u0631\u0628\u064a (Arabic)");
    if (current == AppLanguage::English) m_en->setChecked(true);
    else m_ar->setChecked(true);
    gl->addWidget(m_en);
    gl->addWidget(m_ar);
    vl->addWidget(langGrp);

    // ── Appearance group ──────────────────────────────────────────────────
    auto* appGrp = new QGroupBox(T("Appearance","\u0627\u0644\u0645\u0638\u0647\u0631"));
    auto* al = new QVBoxLayout(appGrp);
    al->setSpacing(12);

    m_lightCheck = new QCheckBox(T("Light Mode","\u0627\u0644\u0648\u0636\u0639 \u0627\u0644\u0641\u0627\u062a\u062d"));
    m_lightCheck->setChecked(lightMode);
    al->addWidget(m_lightCheck);

    m_classicViewCheck = new QCheckBox(T("Classic Table View (spreadsheet layout)",
                                         "\u0639\u0631\u0636 \u0627\u0644\u062c\u062f\u0648\u0644 \u0627\u0644\u0643\u0644\u0627\u0633\u064a\u0643\u064a"));
    m_classicViewCheck->setChecked(classicView);
    m_classicViewCheck->setToolTip(T(
        "Switch between the card-based input view (default) and the classic spreadsheet table.",
        "\u0627\u0644\u062a\u0628\u062f\u064a\u0644 \u0628\u064a\u0646 \u0639\u0631\u0636 \u0627\u0644\u0628\u0637\u0627\u0642\u0627\u062a \u0648\u0627\u0644\u062c\u062f\u0648\u0644 \u0627\u0644\u0643\u0644\u0627\u0633\u064a\u0643\u064a."));
    al->addWidget(m_classicViewCheck);

    // Font size row
    {
        auto* row = new QHBoxLayout;
        auto* lbl = new QLabel(T("Text Size:", "\u062d\u062c\u0645 \u0627\u0644\u062e\u0637:"));
        lbl->setStyleSheet("background:transparent; ");
        m_fontSizeCombo = new QComboBox;
        m_fontSizeCombo->addItem(T("Normal (12px)", "\u0639\u0627\u062f\u064a (12px)"), 12);
        m_fontSizeCombo->addItem(T("Large (14px)",  "\u0643\u0628\u064a\u0631 (14px)"),  14);
        m_fontSizeCombo->addItem(T("Extra Large (16px)", "\u0623\u0643\u0628\u0631 (16px)"), 16);
        // Select current
        for (int i = 0; i < m_fontSizeCombo->count(); ++i) {
            if (m_fontSizeCombo->itemData(i).toInt() == fontSize) {
                m_fontSizeCombo->setCurrentIndex(i);
                break;
            }
        }
        row->addWidget(lbl);
        row->addWidget(m_fontSizeCombo, 1);
        al->addLayout(row);
    }

    vl->addWidget(appGrp);

    // ── Currency group ────────────────────────────────────────────────────
    auto* curGrp = new QGroupBox(T("Currency", "\u0627\u0644\u0639\u0645\u0644\u0629"));
    auto* cl = new QVBoxLayout(curGrp);
    cl->setSpacing(12);
    m_usd = new QRadioButton(T("US Dollar ($)", "\u062f\u0648\u0644\u0627\u0631 \u0623\u0645\u0631\u064a\u0643\u064a ($)"));
    m_iqd = new QRadioButton(T("Iraqi Dinar (IQD)", "\u062f\u064a\u0646\u0627\u0631 \u0639\u0631\u0627\u0642\u064a (IQD)"));
    if (currency == AppCurrency::USD) m_usd->setChecked(true);
    else m_iqd->setChecked(true);
    cl->addWidget(m_usd);
    cl->addWidget(m_iqd);
    vl->addWidget(curGrp);

    // ── Buttons ───────────────────────────────────────────────────────────
    auto* bl = new QHBoxLayout;
    auto* cancel = new QPushButton(T("Cancel","\u0625\u0644\u063a\u0627\u0621"));
    cancel->setObjectName("cancel");
    auto* ok = new QPushButton(T("Apply","\u062a\u0637\u0628\u064a\u0642"));
    bl->addWidget(cancel);
    bl->addWidget(ok);
    vl->addLayout(bl);

    connect(ok,     &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
}

AppLanguage SettingsDialog::selectedLanguage() const {
    return m_ar->isChecked() ? AppLanguage::Arabic : AppLanguage::English;
}

bool SettingsDialog::isLightMode() const {
    return m_lightCheck->isChecked();
}

bool SettingsDialog::isClassicView() const {
    return m_classicViewCheck->isChecked();
}

AppCurrency SettingsDialog::selectedCurrency() const {
    return m_iqd->isChecked() ? AppCurrency::IQD : AppCurrency::USD;
}

int SettingsDialog::selectedFontSize() const {
    return m_fontSizeCombo->currentData().toInt();
}
