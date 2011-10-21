#include <QDebug>

#ifdef INTERFACE_COMMANDLINE
#include <QCoreApplication>
#include "CLI/CLI.h"
#endif

#ifdef INTERFACE_GRAPHICAL
#include <QApplication>
#include <QSettings>
#include "UI/MainWindow.h"
#endif

int main(int argc, char *argv[]) {
#ifdef INTERFACE_COMMANDLINE
    QCoreApplication app(argc, argv);

    CLI * cli = new CLI();
    if (!cli->start())
        exit(1);

    return app.exec();
#endif

#ifdef INTERFACE_GRAPHICAL
    const int RESTART_CODE = 1000;
    int r;

    do {
        QApplication app(argc, argv);

        QCoreApplication::setOrganizationName("WimLeers");
        QCoreApplication::setOrganizationDomain("wimleers.com");
        QCoreApplication::setApplicationName("WPO Analytics");

        MainWindow * mainWindow = new MainWindow();
        mainWindow->show();

        r = app.exec();
    } while (r == RESTART_CODE);

    return r;
#endif
}
