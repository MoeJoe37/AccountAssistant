#pragma once
#include <QDialog>
#include <QRadioButton>
#include <QCheckBox>
#include <QComboBox>
#include "translations.h"

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(AppLanguage current, bool lightMode,
                            AppCurrency currency, int fontSize,
                            bool classicView,
                            QWidget* parent = nullptr);
    AppLanguage selectedLanguage() const;
    bool        isLightMode()      const;
    AppCurrency selectedCurrency() const;
    int         selectedFontSize() const;
    bool        isClassicView()    const;
private:
    QRadioButton* m_en{};
    QRadioButton* m_ar{};
    QCheckBox*    m_lightCheck{};
    QCheckBox*    m_classicViewCheck{};
    QRadioButton* m_usd{};
    QRadioButton* m_iqd{};
    QComboBox*    m_fontSizeCombo{};
};
