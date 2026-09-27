// ArduLab application entry point.
//
// Responsibilities (Plan §4.1):
//   - Construct QApplication.
//   - Delegate all composition to ApplicationBootstrap.
//   - Enter the Qt event loop.
// This file contains no catalog, project, or Canvas business logic.

#include "app/ApplicationBootstrap.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("ArduLab"));
    QCoreApplication::setApplicationName(QStringLiteral("ArduLab"));
    QCoreApplication::setApplicationVersion(QStringLiteral(ARDULAB_VERSION_STRING));

    ardulab::app::ApplicationBootstrap bootstrap;
    const int startupCode = bootstrap.start();
    if (startupCode != 0) {
        return startupCode;
    }

    return QApplication::exec();
}
