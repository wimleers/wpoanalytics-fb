#include <QDebug>

#ifdef INTERFACE_COMMANDLINE
#include <QCoreApplication>
#include <qxtcommandoptions.h>
#endif

#ifdef INTERFACE_GRAPHICAL
#include <QApplication>
#include <QSettings>
#include "UI/MainWindow.h"
#endif

int main(int argc, char *argv[]) {
#ifdef INTERFACE_COMMANDLINE
    QCoreApplication app(argc, argv);
    QxtCommandOptions options;
    // Config.
    options.add("config", "config file");
    options.add("verify-config", "verify a config file");
    // State.
    options.add("load", "load state");
    options.alias("load", "l");
    options.add("save", "save state");
    options.alias("save", "s");
    // Data.
    options.add("stdin", "use stdin as input instead of input file (overrides --input)");
    options.add("input", "define input file");
    options.add("stdout", "use stdout as output instead of output file");
    options.add("output", "define output file");
    // Other.
    options.add("help", "Help text -- yet to be written");
    options.alias("help", "h");
    options.parse(QCoreApplication::arguments());
    if (options.count("help") || options.showUnrecognizedWarning()) {
        options.showUsage();
        return -1;
    }
    bool verbose = options.count("verbose");
    int level = 5;
    if (options.count("level")) {
        level = options.value("level").toInt();
    }

    // For now.
    return 0;

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
