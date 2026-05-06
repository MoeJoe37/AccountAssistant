#include <QApplication>
#include <QSurfaceFormat>
#include <QIcon>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTimer>
#include <QMessageLogContext>
#include <cstdlib>
#if defined(Q_OS_WIN)
#  include <windows.h>
#endif
#include "mainwindow.h"

namespace {

static QString startupLogPath()
{
    QString dir;
    if (QCoreApplication::instance())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dir.isEmpty())
        dir = QDir::temp().filePath("AccountAssistant");
    QDir().mkpath(dir);
    return QDir(dir).filePath("startup.log");
}

static void startupMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    QFile file(startupLogPath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        const char* level = "INFO";
        switch (type) {
        case QtDebugMsg:    level = "DEBUG"; break;
        case QtInfoMsg:     level = "INFO"; break;
        case QtWarningMsg:  level = "WARN"; break;
        case QtCriticalMsg: level = "CRITICAL"; break;
        case QtFatalMsg:    level = "FATAL"; break;
        }
        out << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
            << " [" << level << "] " << message;
        if (context.file)
            out << " (" << context.file << ':' << context.line << ')';
        out << '\n';
    }

    if (type == QtFatalMsg)
        std::abort();
}


#if defined(Q_OS_WIN)
static LONG WINAPI accountAssistantUnhandledExceptionFilter(EXCEPTION_POINTERS* info)
{
    QFile file(startupLogPath().replace(QStringLiteral("startup.log"), QStringLiteral("crash.log")));
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " [CRASH] ";
        if (info && info->ExceptionRecord) {
            out << "code=0x" << QString::number(info->ExceptionRecord->ExceptionCode, 16)
                << " address=0x" << QString::number(reinterpret_cast<quintptr>(info->ExceptionRecord->ExceptionAddress), 16);
        } else {
            out << "unknown exception";
        }
        out << '\n';
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

static void hardenWindowsGraphicsStartup()
{
#if defined(Q_OS_WIN)
    // Force the most compatible renderer for Windows 11 machines with hybrid Intel/NVIDIA/AMD GPUs.
    // This avoids driver-specific OpenGL/context creation hangs where the process starts but no window appears.
    qputenv("QT_OPENGL", "software");
    qputenv("QSG_RHI_BACKEND", "software");
    qputenv("QT_QUICK_BACKEND", "software");
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, true);
#endif
}

} // namespace

int main(int argc, char* argv[])
{
    qInstallMessageHandler(startupMessageHandler);
#if defined(Q_OS_WIN)
    SetUnhandledExceptionFilter(accountAssistantUnhandledExceptionFilter);
#endif
    hardenWindowsGraphicsStartup();

    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Set the surface format before QApplication is constructed. Keep it conservative;
    // multisampling is not needed by the app UI and can break startup on some drivers.
    QSurfaceFormat fmt;
    fmt.setSamples(0);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    app.setApplicationName("Account Assistant");
    app.setOrganizationName("AccountAssistant");
    app.setApplicationVersion("7.2.1");

    qInfo() << "Account Assistant startup" << app.applicationVersion()
            << "appDir=" << QCoreApplication::applicationDirPath();

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

#if defined(Q_OS_WIN)
    // A second queued show/raise protects against rare shell/GPU timing issues on startup.
    QTimer::singleShot(150, &win, [&win]() {
        if (!win.isVisible())
            win.showMaximized();
        win.raise();
        win.activateWindow();
    });
#endif

    return app.exec();
}
