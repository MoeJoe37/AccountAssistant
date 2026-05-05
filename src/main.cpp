#include <QApplication>
#include <QSurfaceFormat>
#include <QIcon>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("Account Assistant");
    app.setOrganizationName("AccountAssistant");
    app.setApplicationVersion("7.0.0");

    QSurfaceFormat fmt;
    fmt.setSamples(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    MainWindow win;

    const QStringList iconCandidates = {
        QDir(QCoreApplication::applicationDirPath()).filePath("icon.ico"),
        QDir::current().filePath("icon.ico")
    };
    for (const QString& candidate : iconCandidates) {
        if (QFileInfo::exists(candidate)) {
            QIcon icon(candidate);
            if (!icon.isNull()) {
                app.setWindowIcon(icon);
                win.setWindowIcon(icon);
            }
            break;
        }
    }

    win.showMaximized();
    return app.exec();
}