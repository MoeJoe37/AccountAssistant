#pragma once
#include <QString>
#include <QStringList>

enum class AppLanguage { English, Arabic };
inline AppLanguage g_lang      = AppLanguage::English;
inline bool        g_lightMode = false;

enum class AppCurrency { USD, IQD };
inline AppCurrency g_currency    = AppCurrency::USD;
inline int         g_fontSize    = 12;  // 12, 14, or 16
inline bool        g_classicView = false; // classic spreadsheet table vs. card view

inline QString currencyPrefix()
{
    return g_currency == AppCurrency::USD ? QStringLiteral("$ ") : QStringLiteral("IQD ");
}

inline QString T(const char* en, const char* ar)
{
    return g_lang == AppLanguage::English
        ? QString::fromUtf8(en) : QString::fromUtf8(ar);
}

inline QStringList monthNames()
{
    if (g_lang == AppLanguage::English) {
        return {
            QStringLiteral("January"),
            QStringLiteral("February"),
            QStringLiteral("March"),
            QStringLiteral("April"),
            QStringLiteral("May"),
            QStringLiteral("June"),
            QStringLiteral("July"),
            QStringLiteral("August"),
            QStringLiteral("September"),
            QStringLiteral("October"),
            QStringLiteral("November"),
            QStringLiteral("December")
        };
    }
    return {
        QStringLiteral("يناير"),
        QStringLiteral("فبراير"),
        QStringLiteral("مارس"),
        QStringLiteral("أبريل"),
        QStringLiteral("مايو"),
        QStringLiteral("يونيو"),
        QStringLiteral("يوليو"),
        QStringLiteral("أغسطس"),
        QStringLiteral("سبتمبر"),
        QStringLiteral("أكتوبر"),
        QStringLiteral("نوفمبر"),
        QStringLiteral("ديسمبر")
    };
}