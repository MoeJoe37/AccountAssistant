#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  themebox.h — Shared themed QMessageBox helpers
//
//  Include this header in any .cpp that needs to show a message-box that
//  respects the application's current light/dark theme.
//
//  Usage:
//      #include "themebox.h"
//      ...
//      ThemeBox::info   (this, tr("Title"), tr("All done."));
//      ThemeBox::warn   (this, tr("Title"), tr("Are you sure?"));
//      ThemeBox::critical(this, tr("Title"), tr("Something went wrong."));
//
//  For a Yes/No confirmation:
//      if (ThemeBox::confirm(this, tr("Title"), tr("Really clear?"))
//              == QMessageBox::Yes) { ... }
// ─────────────────────────────────────────────────────────────────────────────

#include <QMessageBox>
#include "translations.h"


namespace ThemeBox {

// ── Shared stylesheet constants ───────────────────────────────────────────────

inline const char* styleDark()
{
    return R"(
QMessageBox {
    background:#12152a;
}
QMessageBox QLabel {
    color:#e6ebff;
}
QMessageBox QPushButton {
    background:#4f86f7;
    color:white;
    font-weight:700;
    border:none;
    border-radius:7px;
    min-width:92px;
    min-height:32px;
    padding:0 18px;
}
QMessageBox QPushButton:hover  { background:#6a9df9; }
QMessageBox QPushButton:pressed{ background:#3a6fe0; }
QMessageBox QPushButton[text="No"],
QMessageBox QPushButton[text="Cancel"] {
    background:#1e2340;
    color:#c8d0ed;
    border:1px solid #2e3455;
}
)";
}

inline const char* styleLight()
{
    return R"(
QMessageBox {
    background:#f4f6fb;
}
QMessageBox QLabel {
    color:#1e2340;
}
QMessageBox QPushButton {
    background:#4f86f7;
    color:white;
    font-weight:700;
    border:none;
    border-radius:7px;
    min-width:92px;
    min-height:32px;
    padding:0 18px;
}
QMessageBox QPushButton:hover  { background:#6a9df9; }
QMessageBox QPushButton:pressed{ background:#3a6fe0; }
QMessageBox QPushButton[text="No"],
QMessageBox QPushButton[text="Cancel"] {
    background:#eef0fa;
    color:#5a6490;
    border:1px solid #dde2f0;
}
)";
}

inline const char* style() { return g_lightMode ? styleLight() : styleDark(); }

inline QString dialogBaseStyle()
{
    return QString::fromUtf8(style());
}

inline void applyDialogTheme(QDialog* dialog)
{
    if (!dialog) return;
    dialog->setStyleSheet(dialogBaseStyle());
}


// ── Helper builders ───────────────────────────────────────────────────────────

inline void info(QWidget* parent, const QString& title, const QString& text,
                 const QString& informative = QString())
{
    QMessageBox box(QMessageBox::Information, title, text,
                    QMessageBox::Ok, parent);
    if (!informative.isEmpty()) box.setInformativeText(informative);
    box.setTextFormat(Qt::PlainText);
    box.setStyleSheet(style());
    box.exec();
}

inline void warn(QWidget* parent, const QString& title, const QString& text,
                 const QString& informative = QString())
{
    QMessageBox box(QMessageBox::Warning, title, text,
                    QMessageBox::Ok, parent);
    if (!informative.isEmpty()) box.setInformativeText(informative);
    box.setTextFormat(Qt::PlainText);
    box.setStyleSheet(style());
    box.exec();
}

inline void critical(QWidget* parent, const QString& title, const QString& text,
                     const QString& informative = QString())
{
    QMessageBox box(QMessageBox::Critical, title, text,
                    QMessageBox::Ok, parent);
    if (!informative.isEmpty()) box.setInformativeText(informative);
    box.setTextFormat(Qt::PlainText);
    box.setStyleSheet(style());
    box.exec();
}

// Returns QMessageBox::Yes or QMessageBox::No.
inline QMessageBox::StandardButton confirm(QWidget* parent,
                                           const QString& title,
                                           const QString& text)
{
    QMessageBox box(QMessageBox::Warning, title, text,
                    QMessageBox::NoButton, parent);
    
    // Add custom buttons with translated text
    QPushButton* yesBtn = box.addButton(tr_yes_abc123(), QMessageBox::YesRole);
    QPushButton* noBtn = box.addButton(tr_no_def456(), QMessageBox::NoRole);
    box.setDefaultButton(noBtn);
    
    box.setStyleSheet(style());
    box.exec();
    
    if (box.clickedButton() == yesBtn) {
        return QMessageBox::Yes;
    }
    return QMessageBox::No;
}

} // namespace ThemeBox
