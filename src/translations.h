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
    if (g_lang == AppLanguage::English)
        return { "Jan","Feb","Mar","Apr","May","Jun",
                 "Jul","Aug","Sep","Oct","Nov","Dec" };
    return {
        "\u064A\u0646\u0627","\u0641\u0628\u0631","\u0645\u0627\u0631",
        "\u0623\u0628\u0631","\u0645\u0627\u064A","\u064A\u0648\u0646",
        "\u064A\u0648\u0644","\u0623\u063A\u0633","\u0633\u0628\u062A",
        "\u0623\u0643\u062A","\u0646\u0648\u0641","\u062F\u064A\u0633"
    };
}