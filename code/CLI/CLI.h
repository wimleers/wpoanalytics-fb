#ifndef CLI_H
#define CLI_H

#include <qxtcommandoptions.h>
#include <QTextStream>

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
    void parse(QString file);

    void finished();

public slots:
    void run();

    void wakeParser();
    void updateParsingStatus(bool parsing);

private:
    void reset();
    void init();
    void initLogic();
    void connectLogic();
    void assignThreads();

    // CLI options.
    QString configFile;
    QString inputFile;
    bool inputStdin;

    // Threads.
    QThread parserThread;
    QThread analystThread;

    // Core components.
    Config::Config * config;
    FacebookLogParser::Parser * parser;
    Analytics::Analyst * analyst;

    // Other.
    QTextStream * out;
};

#endif // CLI_H
