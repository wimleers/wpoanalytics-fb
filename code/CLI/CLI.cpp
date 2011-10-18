#include "CLI.h"


CLI::CLI() {
    this->config = NULL;
    this->parser = NULL;
    this->analyst = NULL;

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

    bool verbose = options.count("verbose");
    if (options.count("config")) {
        this->configFile = options.value("config").toString();
    }
    else {
        options.showUsage();
        return -1;
    }

    return 0;
}

void CLI::run() {
    // DBG
    this->init();

    emit finished();
}

void CLI::reset() {
    this->parser = NULL;
    this->analyst = NULL;
}

void CLI::init() {
    this->config = new Config::Config();
    if (!this->config->parse(this->configFile))
        qDebug() << "parsing failed";
    else {
        qDebug() << *this->config;
    }

    // Instantiate the Parser.
    this->parser = new FacebookLogParser::Parser();

    // Instantiate the Analyst.
    double minSupport = this->config->getMinPatternSupport();
    double minPatternTreeSupport = this->config->getMinPotentialPatternSupport();
    double minConfidence = this->config->getMinRuleConfidence();
    this->analyst = new Analytics::Analyst(minSupport, minPatternTreeSupport, minConfidence);

    // Set pattern & rule consequent constraints. This defines which
    // associations will be found by the Analyst.
    Analytics::ItemConstraintType constraintType;
    Analytics::ItemConstraintsHash frequentItemsetItemConstraints, ruleConsequentItemConstraints;
    frequentItemsetItemConstraints = this->config->getPatternConstraints();
    for (int i = Analytics::CONSTRAINT_POSITIVE_MATCH_ALL; i <= Analytics::CONSTRAINT_NEGATIVE_MATCH_ANY; i++) {
        constraintType = (Analytics::ItemConstraintType) i;
        foreach (const Analytics::ItemName & itemName, frequentItemsetItemConstraints[constraintType])
            this->analyst->addFrequentItemsetItemConstraint(itemName, constraintType);
    }
    ruleConsequentItemConstraints = this->config->getRuleConsequentConstraints();
    for (int i = Analytics::CONSTRAINT_POSITIVE_MATCH_ALL; i <= Analytics::CONSTRAINT_NEGATIVE_MATCH_ANY; i++) {
        constraintType = (Analytics::ItemConstraintType) i;
        foreach (const Analytics::ItemName & itemName, ruleConsequentItemConstraints[constraintType])
            this->analyst->addRuleConsequentItemConstraint(itemName, constraintType);
    }
}
