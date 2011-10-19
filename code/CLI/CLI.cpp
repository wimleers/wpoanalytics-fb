#include "CLI.h"


//---------------------------------------------------------------------------
// Public methods.

CLI::CLI() {
    this->config = NULL;
    this->parser = NULL;
    this->analyst = NULL;
    this->out = new QTextStream(stdout);

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

int CLI::parseCommandOptions() {
    QxtCommandOptions options;
    // Config.
    options.add("config", "Config file (required).", QxtCommandOptions::ValueRequired);
    // State.
    options.add("load", "Load state: continue from the state in this file.", QxtCommandOptions::ValueRequired);
    options.alias("load", "l");
    options.add("save", "Save state; save the state to a file so it can be continued later.", QxtCommandOptions::ValueRequired);
    options.alias("save", "s");
    // Data.
    int inputGroup = 0;
    options.add("input", "Define input file (required).", QxtCommandOptions::ValueRequired, inputGroup);
    options.alias("input", "i");
    options.add("input-stdin", "Use stdin as input (overrides --input).", QxtCommandOptions::NoValue, inputGroup);
    options.alias("input-stdin", "is");
    int outputGroup = 1;
    options.add("output", "Define output file (required).", QxtCommandOptions::ValueRequired, outputGroup);
    options.alias("output", "o");
    options.add("output-stdout", "Use stdout as output (overrides --output).", QxtCommandOptions::NoValue, outputGroup);
    options.alias("output-stdout", "os");
    // Verification.
    options.add("verify-config", "Verify a config file.", QxtCommandOptions::ValueRequired);
    options.add("verify-state", "Verify a state file.", QxtCommandOptions::ValueRequired);
    // Other.
    options.add("help", "Show this help text.");
    options.alias("help", "h");
    options.setParamStyle(QxtCommandOptions::Space);
    options.parse(QCoreApplication::arguments());
    if (options.count("help") || options.unrecognized().size() > 0 || options.showUnrecognizedWarning()) {
        options.showUsage();
        return -1;
    }

//    bool verbose = options.count("verbose");

    bool validUsage = true;
    validUsage = (options.count("config") > 0);
    validUsage = (options.count("input") > 0 || options.count("input-stdin") > 0);

    if (!validUsage) {
        options.showUsage();
        return -1;
    }

    this->configFile = options.value("config").toString();
    if (options.count("input") > 0) {
        this->inputFile = options.value("input").toString();
        this->inputStdin = false;
    }
    else
        this->inputFile = true;

    return 0;
}


//---------------------------------------------------------------------------
// Public slots.

void CLI::run() {
    // DBG
    this->init();

    // Start the parsing, which also will start the analysis.
    if (!this->inputStdin)
        emit parse(this->inputFile);
    else
        emit parse(":stdin"); // Use an illegal filename to identify stdin.

//    emit finished();
}

void CLI::wakeParser() {
    this->parser->continueParsing();
}

void CLI::updateParsingStatus(bool parsing) {
//    QTextStream stdout()
    if (parsing)
        *out << "Parsing ...";
    else
        *out << " done!" << endl;
}


//---------------------------------------------------------------------------
// Private methods.

void CLI::reset() {
    this->parser = NULL;
    this->analyst = NULL;
}

void CLI::init() {
    this->config = new Config::Config();
    if (!this->config->parse(this->configFile))
        qCritical("Failed to parse the config file '%s'.", qPrintable(this->configFile));
    else {
        qDebug() << *this->config;
    }

    this->initLogic();
    this->connectLogic();
}

void CLI::initLogic() {
    // Instantiate the Parser.
    this->parser = new FacebookLogParser::Parser(this->config);

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
    for (int i = Analytics::CONSTRAINT_POSITIVE_MATCH_ALL; i <= Analytics::CONSTRAINT_NEGATIVE_MATCH_ANY; i++) {
        constraintType = (Analytics::ItemConstraintType) i;
        foreach (const Analytics::ItemName & itemName, frequentItemsetItemConstraints[constraintType])
            this->analyst->addFrequentItemsetItemConstraint(itemName, constraintType);
    }
    ruleConsequentItemConstraints = this->config->getRuleConsequentItemConstraints();
    for (int i = Analytics::CONSTRAINT_POSITIVE_MATCH_ALL; i <= Analytics::CONSTRAINT_NEGATIVE_MATCH_ANY; i++) {
        constraintType = (Analytics::ItemConstraintType) i;
        foreach (const Analytics::ItemName & itemName, ruleConsequentItemConstraints[constraintType])
            this->analyst->addRuleConsequentItemConstraint(itemName, constraintType);
    }
}

void CLI::connectLogic() {
    // Pure logic.
    connect(this->parser, SIGNAL(parsedBatch(QList<QStringList>, double, Time, Time, quint32, bool)), this->analyst, SLOT(analyzeTransactions(QList<QStringList>, double, Time, Time, quint32, bool)));

    // Logic -> main thread -> logic (wake up sleeping threads).
    connect(this->analyst, SIGNAL(processedBatch()), this, SLOT(wakeParser()));

    // UI -> logic.
    connect(this, SIGNAL(parse(QString)), this->parser, SLOT(parse(QString)));
}

void CLI::assignThreads() {
    this->parser->moveToThread(&this->parserThread);
    this->analyst->moveToThread(&this->analystThread);

    this->parserThread.start();
    this->analystThread.start();
}
