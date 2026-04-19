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
    setWindowTitle(tr_settings_a53cf0());
    setMinimumWidth(400);

    auto* vl = new QVBoxLayout(this);
    vl->setContentsMargins(28,24,28,24);
    vl->setSpacing(14);

    auto* title = new QLabel(tr_settings_a33f70());
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    vl->addWidget(title);

    // ── Language group ────────────────────────────────────────────────────
    auto* langGrp = new QGroupBox(tr_language_31ad2d());
    auto* gl  = new QVBoxLayout(langGrp);
    gl->setSpacing(12);
    m_en = new QRadioButton(tr_english_7e4a3f());
    m_ar = new QRadioButton(tr_arabic_41c9d8());
    if (current == AppLanguage::English) m_en->setChecked(true);
    else m_ar->setChecked(true);
    gl->addWidget(m_en);
    gl->addWidget(m_ar);
    vl->addWidget(langGrp);

    // ── Appearance group ──────────────────────────────────────────────────
    auto* appGrp = new QGroupBox(tr_appearance_97f3e4());
    auto* al = new QVBoxLayout(appGrp);
    al->setSpacing(12);

    m_lightCheck = new QCheckBox(tr_light_mode_1a03c6());
    m_lightCheck->setChecked(lightMode);
    al->addWidget(m_lightCheck);

    m_classicViewCheck = new QCheckBox(tr_classic_table_view_spreadsheet_b69d40());
    m_classicViewCheck->setChecked(classicView);
    m_classicViewCheck->setToolTip(tr_switch_between_the_card_based__d92db3());
    al->addWidget(m_classicViewCheck);

    // Font size row
    {
        auto* row = new QHBoxLayout;
        auto* lbl = new QLabel(tr_text_size_5b8d4f());
        lbl->setStyleSheet("background:transparent; ");
        m_fontSizeCombo = new QComboBox;
        m_fontSizeCombo->addItem(tr_normal_12px_ac2426(), 12);
        m_fontSizeCombo->addItem(tr_large_14px_57768d(),  14);
        m_fontSizeCombo->addItem(tr_extra_large_16px_3e8432(), 16);
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
    auto* curGrp = new QGroupBox(tr_currency_88f072());
    auto* cl = new QVBoxLayout(curGrp);
    cl->setSpacing(12);
    m_usd = new QRadioButton(tr_us_dollar_105f33());
    m_iqd = new QRadioButton(tr_iraqi_dinar_iqd_c97fa1());
    if (currency == AppCurrency::USD) m_usd->setChecked(true);
    else m_iqd->setChecked(true);
    cl->addWidget(m_usd);
    cl->addWidget(m_iqd);
    vl->addWidget(curGrp);

    // ── Buttons ───────────────────────────────────────────────────────────
    auto* bl = new QHBoxLayout;
    auto* cancel = new QPushButton(tr_cancel_b879b2());
    cancel->setObjectName("cancel");
    auto* ok = new QPushButton(tr_apply_042b05());
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
