#include "CLI.h"


//---------------------------------------------------------------------------
// Public methods.

CLI::CLI() {
    this->config = NULL;
    this->parser = NULL;
    this->analyst = NULL;

    // Status.
    this->parsing = false;
    this->miningPatterns = false;

    // run() flow.
    this->configVerificationCompleted = false;
    this->loadCompleted               = false;
    this->inputCompleted              = false;
    this->saveCompleted               = false;
    this->mineCompleted               = false;

    // Stats: Parser.
    this->statsParserDuration             = 0;
    this->statsParserLines                = 0;
    this->statsParserTransactions         = 0;
    this->statsParserAvgTransactionLength = 0.0;

    // Stats: Analyst: pattern mining.
    this->statsAnalystDuration           = 0;
    this->statsAnalystPatternTreeSize    = 0;
    this->statsAnalystLines              = 0;
    this->statsAnalystTransactions       = 0;
    this->statsAnalystNumFrequentItems   = 0;
    this->statsAnalystNumUniqueItems     = 0;
    this->statsAnalystLoadedLines        = 0;
    this->statsAnalystLoadedTransactions = 0;

    // Stats: Analyst: rule mining.
    this->statsRuleMiningDuration     = 0;
    this->statsRuleMiningLines        = 0;
    this->statsRuleMiningTransactions = 0;
    this->statsRuleMiningPatterns     = 0;

    // Options for which defaults are needed.
    this->optionVerbosity = 0;

    qRegisterMetaType< QList<QStringList> >("QList<QStringList>");
    qRegisterMetaType< QList<float> >("QList<float>");
    qRegisterMetaType<Time>("Time");
    Analytics::registerBasicMetaTypes();
}

CLI::~CLI() {
    if (this->config != NULL)
        delete config;
    if (this->parser != NULL)
        delete this->parser;
    if (this->analyst != NULL)
        delete this->analyst;
}

bool CLI::start() {
    this->out("CLI", "Starting...", 1);
    if (this->parseCommandOptions()) {
        this->initConfig();
        this->initLogic();
        this->connectLogic();
        this->assignThreads();
        this->run();
        return true;
    }
    else
        return false;
}


//---------------------------------------------------------------------------
// Public slots.

void CLI::wakeParser() {
    this->out("CLI", "Parser continuing...", 0);
    this->parser->continueParsing();
}

void CLI::updateParsingStatus(bool parsing) {
    QMutexLocker(&this->statusMutex);
    this->parsing = parsing;

    if (parsing)
        this->out("Parser", "Parsing...", 0);
    else {
        this->out("Parser", "Done.", 0);

        emit this->patterMiningFinished();
    }
}

void CLI::updateParserStats(int duration, quint64 transactions, double transactionsPerEvent, double averageTransactionLength, bool lastChunkOfBatch, Time start, Time end) {
    this->statsMutex.lock();
    this->statsParserDuration             += duration;
    this->statsParserTransactions         += transactions;
    this->statsParserLines                += transactions / transactionsPerEvent;
    double weightedUpToNow = this->statsParserAvgTransactionLength * ((double) (this->statsParserTransactions - transactions) / this->statsParserTransactions);
    double weightedNew = averageTransactionLength * ((double) transactions / this->statsParserTransactions);
    this->statsParserAvgTransactionLength  = weightedUpToNow + weightedNew;
    this->statsMutex.unlock();

    this->out("Parser", "Chunk parsed.", 0);
    this->out(
                "Parser",
                QString(" |- %1 lines/s (%2 s)")
                .arg(QString::number((transactions / transactionsPerEvent) / (duration / 1000.0), 'f', 2))
                .arg(QString::number(duration / 1000.0, 'f', 2)),
                1
    );
    this->out(
                "Parser",
                QString(" |- %1 lines -> %2 transactions (%3 transactions/line)")
                .arg(transactions / transactionsPerEvent)
                .arg(transactions)
                .arg(transactionsPerEvent),
                1
    );
    this->out("Parser", QString(" |- %1 items/transaction").arg(averageTransactionLength), 1);
    this->out(
                "Parser",
                QString(" |- Batch completed: %1. Time range: [%2, %3].")
                .arg(lastChunkOfBatch ? "Yes" : "No")
                .arg(QDateTime::fromTime_t(start).toString("yyyy-MM-dd hh:mm:ss"))
                .arg(QDateTime::fromTime_t(end).toString("yyyy-MM-dd hh:mm:ss")),
                2
    );
    this->out(
                "Parser",
                QString(" \\- Total: %1s, %2 lines, %3 transactions. %4 items/transaction.")
                .arg(QString::number(this->statsParserDuration / 1000.0, 'f', 2))
                .arg(QString::number(this->statsParserLines / 1000.0, 'f', 2) + 'K')
                .arg(QString::number(this->statsParserTransactions / 1000.0, 'f', 2) + 'K')
                .arg(QString::number(this->statsParserAvgTransactionLength, 'f', 2)),
                1
    );
}

void CLI::updatePatternMiningStatus(bool miningPatterns, Time start, Time end, quint64 lines, quint64 transactions) {
    Q_UNUSED(start)
    Q_UNUSED(end)
    Q_UNUSED(lines)
    Q_UNUSED(transactions)

    this->statusMutex.lock();
    this->miningPatterns = miningPatterns;
    this->statusMutex.unlock();

    if (miningPatterns)
        this->out("Analyst", "Mining patterns...", 0);
}

void CLI::updatePatternMiningStats(int duration, Time start, Time end, quint64 pageViews, quint64 transactions, quint64 uniqueItems, quint64 frequentItems, quint64 patternTreeSize) {
    Q_UNUSED(start)
    Q_UNUSED(end)

    this->statsMutex.lock();
    // Chunk-specific.
    this->statsAnalystDuration += duration;
    this->statsAnalystLines += pageViews;
    this->statsAnalystTransactions += transactions;
    // Total.
    this->statsAnalystPatternTreeSize = patternTreeSize;
    this->statsAnalystNumFrequentItems = frequentItems;
    this->statsAnalystNumUniqueItems = uniqueItems;
    this->statsMutex.unlock();

    this->out("Analyst", "Chunk mined for patterns.", 0);
    this->out(
                "Analyst",
                QString(" |- %1 transactions/s or %2 lines/s (%3 s)")
                .arg(QString::number(transactions / (duration / 1000.0), 'f', 2))
                .arg(QString::number(pageViews / (duration / 1000.0), 'f', 2))
                .arg(QString::number(duration / 1000.0, 'f', 2)),
                1
    );
    this->out("Analyst", QString(" |- %1 patterns (in the PatternTree)").arg(this->statsAnalystPatternTreeSize), 1);
    this->out(
                "Analyst",
                QString(" |- %1 frequent items, %2 unique items")
                .arg(this->statsAnalystNumFrequentItems)
                .arg(this->statsAnalystNumUniqueItems),
                2
    );
    this->out(
                "Analyst",
                QString(" \\- Total: %1s, %3 transactions")
                .arg(QString::number(this->statsAnalystDuration / 1000.0, 'f', 2))
                .arg(QString::number((this->statsAnalystTransactions - this->statsAnalystLoadedTransactions) / 1000.0, 'f', 2) + 'K'),
                1
    );
}

void CLI::loaded(bool success, Time start, Time end, quint64 pageViews, quint64 transactions, quint64 uniqueItems, quint64 frequentItems, quint64 patternTreeSize) {
    Q_UNUSED(start)
    Q_UNUSED(end)

    if (!success) {
        this->out("CLI", "Loading failed.", 0);
        this->exit(1);
    }
    else {
        this->loadCompleted = true;

        this->out("Analyst", "Loading successful! Previously analyzed:", 0);
        this->out("Analyst", QString(" |- lines: %1").arg(pageViews), 0);
        this->out("Analyst", QString(" |- transactions: %1").arg(transactions), 0);
        this->out("Analyst", QString(" |- frequent items: %1, unique items: %2").arg(frequentItems).arg(uniqueItems), 0);
        this->out("Analyst", QString(" \\- patterns: %1").arg(patternTreeSize), 0);

        this->statsMutex.lock();
        this->statsAnalystLoadedLines = pageViews;
        this->statsAnalystLoadedTransactions = transactions;
        this->statsAnalystPatternTreeSize = patternTreeSize;
        this->statsMutex.unlock();

        this->run();
    }

}

void CLI::saved(bool success) {
    if (!success) {
        this->out("CLI", "Saving failed.", 0);
        this->exit(1);
    }
    else {
        this->saveCompleted = true;
        this->out("CLI", "Saving successful!", 0);
        this->run();
    }
}

void CLI::updateRuleMiningStatus(bool mining) {
    this->statusMutex.lock();
    this->miningRules = mining;
    this->statusMutex.unlock();

    if (miningPatterns)
        this->out("Analyst", "Mining association rules...", 0);
}

void CLI::updateRuleMiningStats(int duration, Time start, Time end, quint64 numAssociationRules, quint64 numTransactions, quint64 numLines) {
    Q_UNUSED(start)
    Q_UNUSED(end)

    this->statsMutex.lock();
    this->statsRuleMiningDuration     += duration;
    this->statsRuleMiningLines        += numLines;
    this->statsRuleMiningTransactions += numTransactions;
    this->statsRuleMiningPatterns     += this->statsAnalystPatternTreeSize;
    this->statsMutex.unlock();

    this->out("Analyst", "Rules mined.", 0);
    this->out(
                "Analyst",
                QString(" |- %1 patterns/s, %2 transactions/s or %3 lines/s (%4 s)")
                .arg(QString::number(this->statsAnalystPatternTreeSize / (duration / 1000.0) / 1000.0, 'f', 2) + 'K')
                .arg(QString::number(numTransactions / (duration / 1000.0) / 1000.0 / 1000.0, 'f', 2) + 'M')
                .arg(QString::number(numLines / (duration / 1000.0) / 1000.0 / 1000.0, 'f', 2) + 'M')
                .arg(QString::number(duration / 1000.0, 'f', 2)),
                1
    );
    this->out(
                "Analyst",
                QString(" |- %1 patterns -> %2 assocation rules")
                .arg(this->statsAnalystPatternTreeSize)
                .arg(numAssociationRules),
                1
    );
    this->out(
                "Analyst",
                QString(" \\- Total: %1 ms, %2 patterns/s, %3 patterns, %4 lines, %5 transactions.")
                .arg(this->statsRuleMiningDuration)
                .arg(QString::number(this->statsRuleMiningPatterns / (this->statsRuleMiningDuration / 1000.0) / 1000.0, 'f', 2) + 'K')
                .arg(QString::number(this->statsRuleMiningPatterns / 1000.0, 'f', 2) + 'K')
                .arg(QString::number(this->statsRuleMiningLines / 1000.0 / 1000.0, 'f', 2) + 'M')
                .arg(QString::number(this->statsRuleMiningTransactions / 1000.0, 'f', 2) + 'K'),
                1
    );

}

void CLI::minedRules(uint from, uint to, QList<Analytics::AssociationRule> associationRules, Analytics::SupportCount eventsInTimeRange) {
    Q_UNUSED(from)
    Q_UNUSED(to)
    Q_UNUSED(eventsInTimeRange)


    QFile file;
    bool opened = false;

    if (this->optionOutputStdout) {
        this->out("CLI", "Saving rules to stdout.", 0);
        opened = file.open(stdout, QIODevice::WriteOnly | QIODevice::Text);
    }
    else {
        this->out("CLI", QString("Saving rules to '%1'.").arg(this->optionOutputFile), 0);
        file.setFileName(this->optionOutputFile);
        opened = file.open(QIODevice::WriteOnly | QIODevice::Text);
    }

    if (!opened) {
        qCritical("Could not open file %s for writing.", qPrintable(this->optionOutputFile));
    }
    else {
        QTextStream out(&file);

        QVariantMap ruleJSON;
        foreach (const Analytics::AssociationRule & rule, associationRules) {
            QVariantList antecedentJSON;
            foreach (const Analytics::ItemName & itemName, this->analyst->itemsetIDsToNames(rule.antecedent))
                antecedentJSON.append((QString) itemName);

            QVariantList consequentJSON;
            foreach (const Analytics::ItemName & itemName, this->analyst->itemsetIDsToNames(rule.consequent))
                consequentJSON.append((QString) itemName);

            ruleJSON.insert("antecedent", antecedentJSON);
            ruleJSON.insert("consequent", consequentJSON);
            ruleJSON.insert("support", (int) rule.support);
            ruleJSON.insert("confidence", (double) rule.confidence);

            out << QxtJSON::stringify(ruleJSON) << "\n";

            ruleJSON.clear();
        }

        file.close();
    }

    this->mineCompleted = true;
    this->run();
}



//---------------------------------------------------------------------------
// Private slots.

void CLI::patterMiningFinished() {
    QString startBold = "\033[7m";
    QString stopBold = "\033[0m";

    // Final stats.
    this->out("Stats", startBold + "Parser" + stopBold, 0);
    this->out(
                "Stats",
                QString(" |- %1 lines/s (%2 s)")
                .arg(QString::number(this->statsParserLines / (this->statsParserDuration / 1000.0), 'f', 2))
                .arg(QString::number(this->statsParserDuration / 1000.0, 'f', 2)),
                0
    );
    this->out(
                "Stats",
                QString(" |- %1 lines -> %2 transactions (%3 transactions/line)")
                .arg(this->statsParserLines)
                .arg(this->statsParserTransactions)
                .arg(1.0 * this->statsParserTransactions / this->statsParserLines),
                0
    );
    this->out("Stats", QString(" |- %1 items/transaction").arg(this->statsParserAvgTransactionLength), 0);
    this->out(
                "Stats",
                QString(" \\- %1 transactions/s or %2 items/s")
                .arg(QString::number(this->statsParserTransactions / (this->statsParserDuration / 1000.0) / 1000.0, 'f', 2) + 'K')
                .arg(QString::number(this->statsParserAvgTransactionLength * this->statsParserTransactions / (this->statsParserDuration / 1000.0) / 1000.0, 'f', 2) + 'K'),
                0
    );
    this->out("Stats", startBold + "Analyst" + stopBold, 0);
    this->out(
                "Stats",
                QString(" |- %1 lines/s (%2 s)")
                .arg(QString::number(this->statsAnalystLines / (this->statsAnalystDuration / 1000.0), 'f', 2))
                .arg(QString::number(this->statsAnalystDuration / 1000.0, 'f', 2)),
                0
    );
    this->out(
                "Stats",
                QString(" |- %1 transactions -> %2 patterns (%3 patterns/1000 transactions)")
                .arg(this->statsAnalystTransactions)
                .arg(this->statsAnalystPatternTreeSize)
                .arg(QString::number(this->statsAnalystPatternTreeSize / (this->statsAnalystTransactions / 1000.0), 'f', 2)),
                0
    );
    this->out(
                "Stats",
                QString(" |- %1 frequent items, %2 unique items")\
                .arg(this->statsAnalystNumFrequentItems)
                .arg(this->statsAnalystNumUniqueItems),
                0
    );
    this->out(
                "Stats",
                QString(" \\- %1 transactions/s or %2 items/s")
                .arg(QString::number(this->statsAnalystTransactions / (this->statsAnalystDuration / 1000.0) / 1000.0, 'f', 2) + 'K')
                .arg(QString::number(this->statsAnalystTransactions * this->statsParserAvgTransactionLength / (this->statsAnalystDuration/ 1000.0) / 1000.0, 'f', 2) + 'K'),
                0
    );
    quint64 totalDuration = this->statsParserDuration + this->statsAnalystDuration;
    this->out(
                "Stats",
                QString(startBold + "TOTAL: %1 lines/s, %2 transactions/s, %3 items/s (%4s)" + stopBold)
                .arg(QString::number(this->statsAnalystLines / (totalDuration / 1000.0), 'f', 2))
                .arg(QString::number(this->statsAnalystTransactions / (totalDuration / 1000.0) / 1000.0, 'f', 2) + 'K')
                .arg(QString::number(this->statsAnalystTransactions * this->statsParserAvgTransactionLength / (totalDuration / 1000.0) / 1000.0, 'f', 2) + 'K')
                .arg(QString::number(totalDuration / 1000.0, 'f', 2)),
                0
    );

    this->inputCompleted = true;
    this->run();
}


//---------------------------------------------------------------------------
// Private methods (CLI functionality).

bool CLI::parseCommandOptions() {
    QxtCommandOptions options;
    // Config.
    options.add("config", "Config file (required).", QxtCommandOptions::ValueRequired);
    // State.
    options.add("load", "Load pattern tree: continue from the state in this file.", QxtCommandOptions::ValueRequired);
    options.alias("load", "l");
    options.add("save", "Save pattern tree; save the state to a file so it can be continued later.", QxtCommandOptions::ValueRequired);
    options.alias("save", "s");
    // Data.
    int inputGroup = 0;
    options.add("input", "Define input file (required).", QxtCommandOptions::ValueRequired, inputGroup);
    options.alias("input", "i");
    options.add("input-stdin", "Use stdin as input (overrides --input).", QxtCommandOptions::NoValue, inputGroup);
    options.alias("input-stdin", "is");
    // Rules.
    options.add("rules", "Mine rules (defaults to the entire data set", QxtCommandOptions::NoValue);
    int outputGroup = 1;
    options.add("output", "Define output file for storing the mined rules (required if --rules is set).", QxtCommandOptions::ValueRequired, outputGroup);
    options.alias("output", "o");
    options.add("output-stdout", "Use stdout as output for storing the mined rules (overrides --output).", QxtCommandOptions::NoValue, outputGroup);
    options.alias("output-stdout", "os");
    // Verification.
    options.add("verify-config", "Verify a config file.", QxtCommandOptions::NoValue);
    options.add("verify-state", "Verify a state file.", QxtCommandOptions::NoValue);
    // Other.
    int verbosityGroup = 2;
    options.add("quiet", "Zero output about the current state.", QxtCommandOptions::NoValue, verbosityGroup);
    options.alias("quiet", "q");
    options.add("verbose", "Show more information about the process; specify twice for more detail.", QxtCommandOptions::AllowMultiple, verbosityGroup);
    options.alias("verbose", "v");
    options.add("help", "Show this help text.");
    options.alias("help", "h");
    options.setParamStyle(QxtCommandOptions::Space);
    options.parse(QCoreApplication::arguments());
    if (options.count("help") || options.unrecognized().size() > 0 || options.showUnrecognizedWarning()) {
        this->showHelpText();
        options.showUsage();
        return false;
    }

    // Validate & parse general usage.
    bool validUsage = true;
    validUsage = (options.count("config"));
    validUsage = (options.count("input") || options.count("input-stdin") || options.count("load"));
    if (!validUsage) {
        this->showHelpText();
        options.showUsage();
        return false;
    }
    this->optionConfigFile = options.value("config").toString();
    this->optionInput = false;
    if (options.count("input") > 0) {
        this->optionInput = true;
        this->optionInputStdin = false;
        this->optionInputFile = options.value("input").toString();
    }
    else if (options.count("input-stdin")) {
        this->optionInput = true;
        this->optionInputStdin = true;
    }

    // Optional functionality: loading.
    if (options.count("load") > 0) {
        this->optionLoad = true;
        this->optionLoadFile = options.value("load").toString();
    }
    else
        this->optionLoad = false;

    // Optional functionality: saving.
    if (options.count("save") > 0) {
        this->optionSave = true;
        this->optionSaveFile = options.value("save").toString();
    }
    else
        this->optionSave = false;

    // Optional functionality: rule mining.
    this->optionMineRules = false;
    if (options.count("rules") > 0) {
        this->optionMineRules = true;
        if (options.count("output") > 0) {
            this->optionOutput = true;
            this->optionOutputStdout = false;
            this->optionOutputFile = options.value("output").toString();
        }
        else if (options.count("output-stdout")) {
            this->optionOutput = true;
            this->optionOutputStdout = true;
        }
    }

    // Other.
    this->optionVerifyConfig = (options.count("verify-config") > 0);

    // Additional options.
    if (options.count("quiet"))
        this->optionVerbosity = -1;
    else
        this->optionVerbosity = options.count("verbose");

    return true;
}

/**
 * Note that this function may be called multiple times!
 *
 * For example, in the case of loading a file and then performing computations:
 * it will first call
 */
void CLI::run() {
    /**
     * One-off tasks.
     */

    // Verify config file if requested.
    if (this->optionVerifyConfig) {
        if (!this->configVerificationCompleted) {
            this->out("CLI", "Verifying config file.", 0);
            this->verifyConfig();
            return;
        }
        else
            exit(0);
    }
    // run() will be called again by verifyConfig()

    /**
     * Full possible sequence.
     */

    // Load state if requested.
    if (this->optionLoad && !this->loadCompleted) {
        this->out("CLI", QString("Loading state from '%1'.").arg(this->optionLoadFile), 0);
        emit load(this->optionLoadFile);
        return;
    }
    // run() will be called again by loaded()

    // Parse & mine for patterns if requested.
    if (this->optionInput && !this->inputCompleted) {
        if (!this->optionInputStdin) {
            this->out("CLI", QString("Parser input: '%1'.").arg(this->optionInputFile), 0);
            emit parse(this->optionInputFile);
            return;
        }
        else {
            this->out("CLI", QString("Parser input: stdin."), 0);
            emit parse(":stdin"); // Use an illegal filename to identify stdin.
            return;
        }
    }
    // run() will be called again by patternMiningFinished()

    // Save state if requested.
    if (this->optionSave && !this->saveCompleted) {
        this->out("CLI", QString("Saving state to '%1'.").arg(this->optionSaveFile), 0);
        emit save(this->optionSaveFile);
        return;
    }
    // run() will be called again by saved()

    if (this->optionMineRules && !this->mineCompleted) {
        if (this->analyst->getPatternTreeSize() == 0)
            this->out("CLI", QString("There are zero patterns, hence there is nothing to be found but 42."), 0);
        else {
            this->out("CLI", QString("Mining for association rules over %1 patterns...").arg(this->analyst->getPatternTreeSize()), 0);
            emit mine(0, TTW_NUM_BUCKETS-1);
            return;
        }
    }

    // All done! :)
    this->exit(0);
}

void CLI::verifyConfig() {
    // TODO: make this work with QTextStream.
#ifndef DEBUG
    this->out("CLI", "This unfortunately only works in debug mode right now. Sorry about that.", 0);
#endif
    qDebug() << *this->config;
    this->run();
}


//---------------------------------------------------------------------------
// Private methods (helpers).

void CLI::showHelpText() {
    QTextStream out(stdout);
    QString indent = "    ";
    QString startBold = "\033[7m";
    QString stopBold = "\033[0m";

    out << startBold << "PatternMiner" << stopBold << " prototype (" << "\033[33m" << "Project AwesomeLlama" << "\033[0m" << ")" << endl;
    out << endl << endl;

    out << startBold << "Examples" << stopBold << endl;
    out << indent << "1. Mine TTI-related patterns from a dump of the perfpipe_cavalry dataset and saving the resulting state, so we can continue with a subsequent dump later. Also mine rules." << endl;
    out << indent << indent << "PatternMiner --config ~/perfpipe_cavalry/tti_config.json --input ~/perfpipe_cavalry/dump.json --save ~/perfpipe_cavalry/state.pf -vv" << endl;
    out << endl;
    out << indent << "2. Load a state we saved before and mine association rules from it." << endl;
    out << indent << indent << "PatternMiner --config ~/perfpipe_cavalry/tti_config.json --load ~/perfpipe_cavalry/state.pf --rules --output-stdout -vv" << endl;
    out << endl;
    out << indent << "3. The same, but this time store the asociation rules in a file." << endl;
    out << indent << indent << "PatternMiner --config ~/perfpipe_cavalry/tti_config.json --load ~/perfpipe_cavalry/state.pf --rules --output ~/perfpipe_cavalry_rules.json -vv" << endl;
    out << endl << endl;

    out << startBold << "Options" << stopBold << endl;

    out.flush();
}

void CLI::out(const QString & module, const QString & output, int verbosity) {
    static QTextStream out(stdout);
    static QString startBold = "\033[7m";
    static QString stopBold = "\033[0m";


    // Ignore too verbose messages.
    if (verbosity > this->optionVerbosity)
        return;

    out << QString(startBold + "[%1]" + stopBold + " ").arg(module, -7) << output << endl;
}

void CLI::exit(int returnCode) {
    // quit() or terminate()?
    this->parserThread.quit();
    this->parserThread.wait();
    this->analystThread.quit();
    this->analystThread.wait();

    QCoreApplication::instance()->exit(returnCode);
}


//---------------------------------------------------------------------------
// Private methods (logic).

void CLI::reset() {
    this->parser = NULL;
    this->analyst = NULL;
}

void CLI::initConfig() {
    this->config = new Config::Config();
    if (!this->config->parse(this->optionConfigFile))
        qCritical("Failed to parse the config file '%s'.", qPrintable(this->optionConfigFile));
}

void CLI::initLogic() {
    // Instantiate the Parser.
    this->parser = new FacebookLogParser::Parser(*this->config);

    // Instantiate the Analyst.
    double minSupport = this->config->getMinPatternSupport();
    double minPatternTreeSupport = this->config->getMinPotentialPatternSupport();
    double minConfidence = this->config->getMinRuleConfidence();
    this->analyst = new Analytics::Analyst(minSupport, minPatternTreeSupport, minConfidence);

    // Set pattern & rule consequent constraints. This defines which
    // associations will be found by the Analyst.
    Analytics::ItemConstraintType constraintType;
    Analytics::ItemConstraintsHash frequentItemsetItemConstraints, ruleConsequentItemConstraints;
    frequentItemsetItemConstraints = this->config->getPatternItemConstraints();
    for (int i = Analytics::CONSTRAINT_POSITIVE; i <= Analytics::CONSTRAINT_NEGATIVE; i++) {
        constraintType = (Analytics::ItemConstraintType) i;
        foreach (const QSet<Analytics::ItemName> & items, frequentItemsetItemConstraints[constraintType])
            this->analyst->addFrequentItemsetItemConstraint(items, constraintType);
    }
    ruleConsequentItemConstraints = this->config->getRuleConsequentItemConstraints();
    for (int i = Analytics::CONSTRAINT_POSITIVE; i <= Analytics::CONSTRAINT_NEGATIVE; i++) {
        constraintType = (Analytics::ItemConstraintType) i;
        foreach (const QSet<Analytics::ItemName> & items, ruleConsequentItemConstraints[constraintType])
            this->analyst->addRuleConsequentItemConstraint(items, constraintType);
    }
}


void CLI::connectLogic() {
    // Pure logic.
    connect(this->parser, SIGNAL(parsedBatch(QList<QStringList>, double, Time, Time, quint32, bool)), this->analyst, SLOT(analyzeTransactions(QList<QStringList>, double, Time, Time, quint32, bool)));

    // Logic -> main thread -> logic (wake up sleeping threads).
    connect(this->analyst, SIGNAL(processedBatch()), this, SLOT(wakeParser()));

    // Logic -> UI.
    connect(this->parser, SIGNAL(parsing(bool)), SLOT(updateParsingStatus(bool)));
    connect(this->parser, SIGNAL(stats(int,quint64,double,double,bool,Time,Time)), SLOT(updateParserStats(int,quint64,double,double,bool,Time,Time)));
    connect(this->analyst, SIGNAL(analyzing(bool,Time,Time,quint64,quint64)), SLOT(updatePatternMiningStatus(bool,Time,Time,quint64,quint64)));
    connect(this->analyst, SIGNAL(stats(int,Time,Time,quint64,quint64,quint64,quint64,quint64)), SLOT(updatePatternMiningStats(int,Time,Time,quint64,quint64,quint64,quint64,quint64)));
    connect(this->analyst, SIGNAL(mining(bool)), SLOT(updateRuleMiningStatus(bool)));
    connect(this->analyst, SIGNAL(ruleMiningStats(int,Time,Time,quint64,quint64,quint64)), SLOT(updateRuleMiningStats(int,Time,Time,quint64,quint64,quint64)));

    connect(this->analyst, SIGNAL(loaded(bool,Time,Time,quint64,quint64,quint64,quint64,quint64)), SLOT(loaded(bool,Time,Time,quint64,quint64,quint64,quint64,quint64)));
    connect(this->analyst, SIGNAL(saved(bool)), SLOT(saved(bool)));

    connect(this->analyst, SIGNAL(minedRules(uint,uint,QList<Analytics::AssociationRule>,Analytics::SupportCount)), SLOT(minedRules(uint,uint,QList<Analytics::AssociationRule>,Analytics::SupportCount)));

    // UI -> logic.
    connect(this, SIGNAL(parse(QString)), this->parser, SLOT(parse(QString)));
    connect(this, SIGNAL(mine(uint,uint)), this->analyst, SLOT(mineRules(uint,uint)));
//    connect(this, SIGNAL(mineAndCompare(uint,uint,uint,uint)), this->analyst, SLOT(mineAndCompareRules(uint,uint,uint,uint)));
    connect(this, SIGNAL(load(QString)), this->analyst, SLOT(load(QString)));
    connect(this, SIGNAL(save(QString)), this->analyst, SLOT(save(QString)));
}

void CLI::assignThreads() {
    this->parser->moveToThread(&this->parserThread);
    this->analyst->moveToThread(&this->analystThread);

    this->parserThread.start();
    this->analystThread.start();
}
