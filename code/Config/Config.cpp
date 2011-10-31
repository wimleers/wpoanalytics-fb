#include "Config.h"

namespace Config {
    Config::Config() {
    }

    bool Config::parse(const QString & fileName) {
        this->fileName = fileName;

        QFile file;

        file.setFileName(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        else {
            QString rawJSON = file.readAll();
            QVariantMap json = QxtJSON::parse(rawJSON).toMap();

            // Parser.
            const QVariantMap & parserJSON = json["parser"].toMap();
            this->parserItemConstraints = Config::parseConstraints(parserJSON, "categorical item constraints");

            // Query.
            const QVariantMap & queryJSON = json["query"].toMap();
            // Query: patterns.
            const QVariantMap & patternsJSON = queryJSON["patterns"].toMap();
            this->minPatternSupport             = Config::parseDouble(patternsJSON, "minimum support");
            this->minPotentialPatternSupport    = Config::parseDouble(patternsJSON, "minimum potential pattern support");
            this->patternItemConstraints        = Config::parseConstraints(patternsJSON, "item constraints");
            // Query: association rules.
            const QVariantMap & associationRulesJSON = queryJSON["association rules"].toMap();
            this->minRuleConfidence             = Config::parseDouble(associationRulesJSON, "minimum confidence");
            this->ruleAntecedentItemConstraints = Config::parseConstraints(associationRulesJSON, "antecedent item constraints");
            this->ruleConsequentItemConstraints = Config::parseConstraints(associationRulesJSON, "consequent item constraints");

            // Attributes.
            const QVariantMap & attributesJSON = json["attributes"].toMap();
            const QVariantMap & categoricalAttributes = attributesJSON["categorical"].toMap();
            const QVariantMap & numericalAttributes   = attributesJSON["numerical"].toMap();
            QString field;
            foreach (const QVariant & key, categoricalAttributes.keys()) {
                field = key.toString();
                this->categoricalAttributes.insert(field, Config::parseAttribute(categoricalAttributes, field));
            }
            foreach (const QVariant & key, numericalAttributes.keys()) {
                field = key.toString();
                this->numericalAttributes.insert(field, Config::parseAttribute(numericalAttributes, field));
            }
        }

        return true;
    }

    EpisodeSpeed Config::discretize(const EpisodeName & episodeName,
                                                       EpisodeDuration duration,
                                                       Circumstances circumstances
                                                       ) const
    {
        Q_ASSERT(this->numericalAttributes[episodeName].discretizations.size() > 0);

        // Are there conditional discretizations (different discretizations
        // depending on the circumstances), or is there just one?
        bool conditional = this->numericalAttributes[episodeName].discretizations.size() > 1;

        Discretization discretization;

        // Only one discretization: use this one.
        if (!conditional) {
            discretization = this->numericalAttributes[episodeName].discretizations.toList()[0];
        }
        // Find the most specific matching circumstances.
        else {
            QSet<Discretization> matchingDiscretizations = this->numericalAttributes[episodeName].discretizations;
            QMutableSetIterator<Discretization> i(matchingDiscretizations);
            while (i.hasNext()) {
                Discretization d = i.next();
                if (d.circumstances.intersect(circumstances).isEmpty())
                    i.remove();
            }

            // If we *still* have multiple matching discretizations, sort by
            // size and then pick the first one. If multiple circumstances have
            // the same size, the last one is picked.
            // TRICKY: this rather weird behavior is acceptable because it's up
            // to the user to ensure there is no overlap between the different
            // discretizations! This is all courtesy, to ensure that it doesn't
            // break.
            if (matchingDiscretizations.size() >= 2) {
                QMap<int, Discretization> byCircumstanceCounts;
                foreach (const Discretization & d, matchingDiscretizations)
                    byCircumstanceCounts.insert(d.circumstances.size(), d);
                QList<int> circumstanceCounts = byCircumstanceCounts.keys();
                qSort(circumstanceCounts.begin(), circumstanceCounts.end(), qGreater<int>());
                int maxCircumstanceCount = circumstanceCounts[0];
                discretization = byCircumstanceCounts[maxCircumstanceCount];
            }
            // If no discretizations matched the circumstances, then we use the
            // default discretization (which doesn't have any associated
            // circumstances).
            else if (matchingDiscretizations.isEmpty()) {
                QSet<Discretization> discretizations = this->numericalAttributes[episodeName].discretizations;
                QMutableSetIterator<Discretization> i(discretizations);
                while (i.hasNext()) {
                    const Discretization & d = i.next();
                    if (d.isDefault()) {
                        discretization = d;
                        break;
                    }
                }
            }
            // Just one matching discretization.
            else
                discretization = matchingDiscretizations.toList()[0];
        }


        // Now we have a discretization to apply. Apply it.
        EpisodeDuration maxDuration;
        foreach (maxDuration, discretization.thresholds.keys())
            if (duration <= maxDuration)
                return discretization.thresholds[maxDuration];

        qCritical("The duration %d for the Episode '%s' could not be mapped to a discretized speed.", duration, qPrintable(episodeName));
        static QString compilerSatisfaction = QString("satisfy the compiler");
        return compilerSatisfaction;
    }


    //------------------------------------------------------------------------
    // Static protected methods.

    double Config::parseDouble(const QVariantMap & json, const QString & key) {
        return json[key].toDouble();
    }

    Analytics::ItemConstraintsHash Config::parseConstraints(const QVariantMap & json, const QString & key) {
        static QHash<QString, Analytics::ItemConstraintType> keyMapping;

        // Fill the key mapping.
        // (This only needs to happen once, since keyMapping is static.)
        if (keyMapping.isEmpty()) {
            keyMapping.insert("==", Analytics::CONSTRAINT_POSITIVE);
            keyMapping.insert("!=", Analytics::CONSTRAINT_NEGATIVE);
        }

        Analytics::ItemConstraintsHash patternConstraints;
        Analytics::ItemConstraintType itemConstraintType;

        QVariantList relevantJSON = json[key].toList();
        foreach (const QVariant & v, relevantJSON) {
            QVariantMap constraintJSON = v.toMap();

            QString constraintKey = constraintJSON["type"].toString();

            // Ignore non-mapped keys.
            if (!keyMapping.contains(constraintKey))
                continue;

            itemConstraintType = keyMapping[constraintKey];

            // Collect all the items for the constraint.
            QList<QVariant> variantList = constraintJSON["items"].toList();
            QSet<Analytics::ItemName> itemsForConstraint;
            foreach (const QVariant & v, variantList)
                itemsForConstraint.insert((Analytics::ItemName) v.toString());

            // Store it.
            if (!patternConstraints.contains(itemConstraintType))
                patternConstraints.insert(itemConstraintType, QVector<QSet<Analytics::ItemName> >());
            patternConstraints[itemConstraintType].append(itemsForConstraint);
        }

        return patternConstraints;
    }

    Attribute Config::parseAttribute(const QVariantMap & json, const QString & field) {
        QVariantMap relevantJSON = json[field].toMap();

        Attribute a(field);

        // Default to key, which equals the name of the field in the data set.
        if (relevantJSON.keys().contains("name"))
            a.name = relevantJSON["name"].toString();
        else
            a.name = field;

        if (relevantJSON.keys().contains("isEpisode"))
            a.isEpisode = relevantJSON["isEpisode"].toBool();

        if (relevantJSON.keys().contains("parentAttribute"))
            a.parentAttribute = relevantJSON["parentAttribute"].toString();

        if (relevantJSON.keys().contains("hierarchySeparator"))
            a.hierarchySeparator = relevantJSON["hierarchySeparator"].toString();

        if (relevantJSON.keys().contains("discretizations"))
            a.discretizations = Config::parseDiscretizations(relevantJSON["discretizations"], field);

        return a;
    }

    QSet<Discretization> Config::parseDiscretizations(const QVariant & json, const QString & field) {
        QSet<Discretization> discretizations;

        foreach (const QVariant & v, json.toList()) {
            Discretization d;
            QVariantMap relevantJSON = v.toMap();

            if (!relevantJSON.keys().contains("thresholds"))
                qCritical("The 'thresholds' data is missing for the attribute '%s'.", qPrintable(field));
            else {
                QVariantMap thresholdsJSON = relevantJSON["thresholds"].toMap();
                foreach (const EpisodeSpeed & episodeSpeed, thresholdsJSON.keys())
                    d.thresholds.insert(thresholdsJSON[episodeSpeed].toUInt(), episodeSpeed);
            }

            if (relevantJSON.keys().contains("circumstances")) {
                QVariantList circumstancesJSON = relevantJSON["circumstances"].toList();
                foreach (const QVariant & circumstance, circumstancesJSON)
                    d.circumstances.insert(circumstance.toString());
            }

            discretizations.insert(d);
        }

        return discretizations;
    }


    //------------------------------------------------------------------------
    // Other.

    uint qHash(const Discretization & d) {
        QString s;
        foreach (EpisodeDuration episodeDuration, d.thresholds.keys())
            s += d.thresholds[episodeDuration] + '=' + QString::number(episodeDuration) + ':';
        foreach (const EpisodeName & episodeName, d.circumstances)
            s += episodeName + ':';
        return qHash(s);
    }

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const Config & config) {
        dbg.nospace() << "{" << endl;
        dbg.nospace() << "  parser : {" << endl;
        dbg.nospace() << "    \"categorical item constraints\" : {" << endl;
        constraintHashHelper(dbg, config.parserItemConstraints);
        dbg.nospace() << "    }," << endl;
        dbg.nospace() << "  query : {" << endl;
        dbg.nospace() << "    \"patterns\" : {" << endl;
        dbg.nospace() << "      \"minimum support\" : " << config.minPatternSupport << "," << endl;
        dbg.nospace() << "      \"minimum potential pattern support\" : " << config.minPotentialPatternSupport << "," << endl;
        dbg.nospace() << "      \"item constraints\" : {," << endl;
        constraintHashHelper(dbg, config.patternItemConstraints);
        dbg.nospace() << "      }," << endl;
        dbg.nospace() << "    }," << endl;
        dbg.nospace() << "    \"association rules\" : {" << endl;
        dbg.nospace() << "      \"minimum confidence\" : " << config.minRuleConfidence << "," << endl;
        dbg.nospace() << "      \"antecedent item constraints\" : {" << endl;
        constraintHashHelper(dbg, config.ruleAntecedentItemConstraints);
        dbg.nospace() << "      }," << endl;
        dbg.nospace() << "      \"consequent item constraints\" : {" << endl;
        constraintHashHelper(dbg, config.ruleConsequentItemConstraints);
        dbg.nospace() << "      }," << endl;
        dbg.nospace() << "    }," << endl;
        dbg.nospace() << "  }," << endl;
        dbg.nospace() << "  attributes : {" << endl;
        dbg.nospace() << "    categorical : {" << endl;
        foreach (const EpisodeName & episodeName, config.categoricalAttributes.keys())
            attributeHelper(dbg, config.categoricalAttributes[episodeName], "      ");
        dbg.nospace() << "    }," << endl;
        dbg.nospace() << "    numerical : {" << endl;
        foreach (const EpisodeName & episodeName, config.numericalAttributes.keys()) {
            attributeHelper(dbg, config.numericalAttributes[episodeName], "      ");
        }
        dbg.nospace() << "    }," << endl;
        dbg.nospace() << "  }," << endl;

        dbg.nospace() << "}" << endl;

        return dbg.nospace();
    }

    QDebug operator<<(QDebug dbg, const Attribute & attribute) {
        return attributeHelper(dbg, attribute);
    }

    QDebug operator<<(QDebug dbg, const QSet<Discretization> & discretizations) {
        return discretizationsHelper(dbg, discretizations);
    }

    QDebug operator<<(QDebug dbg, const Discretization & discretization) {
        QList<EpisodeDuration> durations = discretization.thresholds.keys();
        qSort(durations.begin(), durations.end());

        dbg.nospace() << "{ ";

        // thresholds
        dbg.nospace() << "thresholds : { ";
        for (int i = 0; i < durations.size(); i++) {
            if (i > 0)
                dbg.nospace() << ", ";
            EpisodeDuration duration = durations[i];
            dbg.nospace() << '"' << discretization.thresholds[duration].toStdString().c_str() << '"';
            dbg.nospace() << " : " << duration;
        }
        dbg.nospace() << " }";

        // circumstances
        dbg.nospace() << ", circumstances : [";
        for (int i = 0; i < discretization.circumstances.size(); i++) {
            if (i > 0)
                dbg.nospace() << ", ";
            dbg.nospace() << discretization.circumstances.toList().at(i);
        }
        dbg.nospace() << "]";
        if (discretization.circumstances.isEmpty())
            dbg.nospace() << " // default";

        dbg.nospace() << " }";

        return dbg.maybeSpace();
    }

    QDebug attributeHelper(QDebug dbg, const Attribute & attribute, const char * prefix) {
        dbg.nospace() << prefix << attribute.field << " : {\n";

        // name
        dbg.nospace() << prefix << "  name              : " << attribute.name << ",";
        if (attribute.field == attribute.name)
            dbg.nospace() << " // default";
        dbg.nospace() << endl;

        // isEpisode
        dbg.nospace() << prefix << "  isEpisode         : " << attribute.isEpisode << ",";
        if (!attribute.isEpisode)
            dbg.nospace() << QString(attribute.name.size() + 2 - 5, ' ').toStdString().c_str() << " // default";
        dbg.nospace() << endl;

        // parentAttribute
        dbg.nospace() << prefix << "  parentAttribute   : " << attribute.parentAttribute << ",";
        if (attribute.parentAttribute.isNull())
            dbg.nospace() << QString(attribute.name.size() + 2 - 2, ' ').toStdString().c_str() << " // default";
        dbg.nospace() << endl;

        // hierarchySeparator
        dbg.nospace() << prefix << "  hierarchySeparator: " << attribute.hierarchySeparator << ",";
        if (attribute.hierarchySeparator.isNull())
            dbg.nospace() << QString(attribute.name.size() + 2 - 2, ' ').toStdString().c_str() << " // default";
        dbg.nospace() << endl;

        // discretizations
        dbg.nospace() << prefix << "  discretizations   : ";
        discretizationsHelper(dbg, attribute.discretizations, QString(QString(prefix) + "  ").toStdString().c_str());
        dbg.nospace() << ',';
        if (attribute.discretizations.isEmpty())
            dbg.nospace() << QString(attribute.name.size() + 2 - 2, ' ').toStdString().c_str() << " // default";
        dbg.nospace() << endl;

        dbg.nospace() << prefix << "}," << endl;

        return dbg.nospace();
    }

    QDebug discretizationsHelper(QDebug dbg, const QSet<Discretization> & discretizations, const char * prefix) {
        QString buffer;

        if (discretizations.isEmpty())
            return dbg.nospace() << "[]";
        else {
            dbg.nospace() << "[" << endl;
            for (int i = 0; i < discretizations.size(); i++) {
                const Discretization d = discretizations.toList().at(i);
                buffer.clear();
                QDebug(&buffer) << d;
                dbg.nospace() << prefix << "  " << buffer.toStdString().c_str() << ",";
                if (i < discretizations.size() - 1)
                    dbg.nospace() << endl;
            }
            dbg.nospace() << endl << prefix << "]";
        }

        return dbg.nospace();
    }

    QDebug constraintHashHelper(QDebug dbg, const QHash<Analytics::ItemConstraintType, QVector<QSet<Analytics::ItemName> > > & hash) {
        if (!hash[Analytics::CONSTRAINT_POSITIVE].isEmpty()) {
            for (int c = 0; c < hash[Analytics::CONSTRAINT_POSITIVE].size(); c++)
                constraintItemsHelper(dbg, hash[Analytics::CONSTRAINT_POSITIVE][c], Analytics::CONSTRAINT_POSITIVE);
        }
        if (!hash[Analytics::CONSTRAINT_NEGATIVE].isEmpty()) {
            for (int c = 0; c < hash[Analytics::CONSTRAINT_NEGATIVE].size(); c++)
                constraintItemsHelper(dbg, hash[Analytics::CONSTRAINT_NEGATIVE][c], Analytics::CONSTRAINT_NEGATIVE);
        }
        return dbg.nospace();
    }

    QDebug constraintItemsHelper(QDebug dbg, const QSet<Analytics::ItemName> & set, Analytics::ItemConstraintType constraintType) {
        bool firstPrinted = false;

        dbg.nospace() << "          {";
        // Type.
        dbg.nospace() << " \"type\" : \"" << (constraintType == Analytics::CONSTRAINT_POSITIVE ? "==" : "!=") << "\", ";
        // Items.
        dbg.nospace() << " \"items\" : [";
        foreach (const Analytics::ItemName & item, set) {
            if (firstPrinted)
                dbg.nospace() << ", ";
            else
                firstPrinted = true;
            dbg.nospace() << item;
        }
        dbg.nospace() << "] }," << endl;

        return dbg.nospace();
    }
#endif

}
