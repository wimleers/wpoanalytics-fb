#ifndef CLI_H
#define CLI_H

#include <qxtcommandoptions.h>

#include "../Config/Config.h"
#include "../FacebookLogParser/Parser.h"
#include "../Analytics/Analyst.h"

class CLI : public QObject {
    Q_OBJECT

public:
    CLI();
    ~CLI();

    int parseCommandOptions();

signals:
    void finished();

public slots:
    void run();

private:
    void reset();
    void init();

    // CLI options.
    QString configFile;

    Config::Config * config;
    FacebookLogParser::Parser * parser;
    Analytics::Analyst * analyst;
};

#endif // CLI_H
