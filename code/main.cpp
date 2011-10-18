#include <QDebug>

#ifdef INTERFACE_COMMANDLINE
#include <QCoreApplication>
//#include <QTimer>
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
//    QObject::connect(cli, SIGNAL(finished()), &app, SLOT(quit()));

    // Parse command-line options. Stop immediately if there's an error.
    if (cli->parseCommandOptions() == -1) {
        delete cli;
        return -1;
    }
    else {
//        QTimer::singleShot(0, cli, SLOT(run()));
        cli->run();
        delete cli;
    }

//    return app.exec();
    return 0;
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
