#include "ui/MainWindow.h"

#include <QApplication>
#include <QSettings>
#include <QSurfaceFormat>
#include <QDir>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    // Enable High-DPI support
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);

    // Application metadata (used by QSettings)
    QApplication::setApplicationName(QStringLiteral("WorldCupAnalyst"));
    QApplication::setOrganizationName(QStringLiteral("WorldCupAnalyst"));
    QApplication::setOrganizationDomain(QStringLiteral("worldcupanalyst.app"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setApplicationDisplayName(
        QStringLiteral("WorldCup Analyst — 2026 FIFA World Cup"));

    // Default OpenGL surface format (improves rendering quality)
    QSurfaceFormat fmt;
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    MainWindow w;
    w.resize(1400, 860);
    w.show();

    return app.exec();
}
