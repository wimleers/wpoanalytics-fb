#include "Config.h"

namespace Config {
    Config::Config() {
    }

    bool Config::parse(const QString & fileName) {
        QFile file;

        file.setFileName(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        else {
            QString rawJSON = file.readAll();
            QVariantMap json = QxtJSON::parse(rawJSON).toMap();

            // Query.
            const QVariantMap & queryJSON = json["query"].toMap();
            this->minPatternSupport          = Config::parseDouble(queryJSON, "minimum pattern support");
            this->minPotentialPatternSupport = Config::parseDouble(queryJSON, "minimum potential pattern support");
            this->minRuleConfidence          = Config::parseDouble(queryJSON, "minimum association rule confidence");
            this->patternConstraints         = Config::parseConstraints(queryJSON, "pattern constraints");
            this->ruleConsequentConstraints  = Config::parseConstraints(queryJSON, "rule consequent constraints");

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

    const FacebookLogParser::EpisodeSpeed & Config::discretize(const FacebookLogParser::EpisodeName & episodeName,
                                                       FacebookLogParser::EpisodeDuration duration,
                                                       QSet<FacebookLogParser::EpisodeName> circumstances
                                                       )
    {
        Q_ASSERT(this->numericalAttributes[episodeName].discretizations.size() > 0);

        // Are there conditional discretizations (different discretizations
        // depending on the circumstances), or is there just one?
        bool conditional = this->numericalAttributes[episodeName].discretizations.size() == 1;

        Discretization discretization;

        // Only one discretization: use this one.
        if (!conditional) {
            discretization = this->numericalAttributes[episodeName].discretizations.toList()[0];
        }
        // Find the most specific matching circumstances.
        else {
            QSet<Discretization> matchingDiscretizations = this->numericalAttributes[episodeName].discretizations;
            qDebug() << "before:" << matchingDiscretizations;
            QMutableSetIterator<Discretization> i(matchingDiscretizations);
            while (i.hasNext()) {
                Discretization d = i.next();
                if (d.circumstances.intersect(circumstances).isEmpty())
                    i.remove();
            }
            qDebug() << "after:" << matchingDiscretizations;

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
                    if (!d.circumstances.isEmpty())
                        i.remove();
                }
                discretization = discretizations.toList()[0];
            }
        }

        qDebug() << "selected discretization: " << discretization;

        // Now we have a discretization to apply. Apply it.
        FacebookLogParser::EpisodeDuration maxDuration;
        foreach (maxDuration, discretization.thresholds.keys()) {
            if (duration <= maxDuration)
                return discretization.thresholds[maxDuration];
        }

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
            keyMapping.insert("positive", Analytics::CONSTRAINT_POSITIVE_MATCH_ANY);
            keyMapping.insert("negative", Analytics::CONSTRAINT_NEGATIVE_MATCH_ANY);
        }

        Analytics::ItemConstraintsHash patternConstraints;
        Analytics::ItemConstraintType itemConstraintType;

        QVariantMap relevantJSON = json[key].toMap();
        foreach (const QString & key, relevantJSON.keys()) {
            // Ignore non-mapped keys.
            if (!keyMapping.contains(key))
                continue;

            itemConstraintType = keyMapping[key];

            // Collect all the items for the constraint.
            QList<QVariant> variantList = relevantJSON[key].toList();
            QSet<Analytics::ItemName> itemsForConstraint;
            foreach (const QVariant & v, variantList)
                itemsForConstraint.insert((Analytics::ItemName) v.toString());
            patternConstraints.insert(itemConstraintType, itemsForConstraint);
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
                foreach (const FacebookLogParser::EpisodeSpeed & episodeSpeed, thresholdsJSON.keys())
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
        foreach (FacebookLogParser::EpisodeDuration episodeDuration, d.thresholds.keys())
            s += d.thresholds[episodeDuration] + '=' + QString::number(episodeDuration) + ':';
        foreach (const FacebookLogParser::EpisodeName & episodeName, d.circumstances)
            s += episodeName + ':';
        return qHash(s);
    }

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const Config & config) {
        dbg.nospace() << "{" << endl;
        dbg.nospace() << "  query : {" << endl;
        dbg.nospace() << "    \"minimum pattern support\" : " << config.minPatternSupport << "," << endl;
        dbg.nospace() << "    \"minimum potential pattern support\" : " << config.minPotentialPatternSupport << "," << endl;
        dbg.nospace() << "    \"minimum association rule confidence\" : " << config.minRuleConfidence << "," << endl;
        dbg.nospace() << "    \"pattern constraints\" : {," << endl;
        constraintHashHelper(dbg, config.patternConstraints);
        dbg.nospace() << "    }," << endl;
        dbg.nospace() << "    \"rule consequent constraints\" : {" << endl;
        constraintHashHelper(dbg, config.ruleConsequentConstraints);
        dbg.nospace() << "    }," << endl;
        dbg.nospace() << "  }," << endl;
        dbg.nospace() << "  attributes : {" << endl;
        dbg.nospace() << "    categorical : {" << endl;
        foreach (const FacebookLogParser::EpisodeName & episodeName, config.categoricalAttributes.keys())
            attributeHelper(dbg, config.categoricalAttributes[episodeName], "      ");
        dbg.nospace() << "    }," << endl;
        dbg.nospace() << "    numerical : {" << endl;
        foreach (const FacebookLogParser::EpisodeName & episodeName, config.numericalAttributes.keys()) {
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
        QList<FacebookLogParser::EpisodeDuration> durations = discretization.thresholds.keys();
        qSort(durations.begin(), durations.end());

        dbg.nospace() << "{ ";

        // thresholds
        dbg.nospace() << "thresholds : { ";
        for (int i = 0; i < durations.size(); i++) {
            if (i > 0)
                dbg.nospace() << ", ";
            FacebookLogParser::EpisodeDuration duration = durations[i];
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

        return dbg.nospace();
    }

    QDebug attributeHelper(QDebug dbg, const Attribute & attribute, const char * prefix) {
        dbg.nospace() << prefix << attribute.field << " : {\n";

        // name
        dbg.nospace() << prefix << "  name            : " << attribute.name << ",";
        if (attribute.field == attribute.name)
            dbg.nospace() << " // default";
        dbg.nospace() << endl;

        // isEpisode
        dbg.nospace() << prefix << "  isEpisode       : " << attribute.isEpisode << ",";
        if (!attribute.isEpisode)
            dbg.nospace() << QString(attribute.name.size() + 2 - 5, ' ').toStdString().c_str() << " // default";
        dbg.nospace() << endl;

        // parentAttribute
        dbg.nospace() << prefix << "  parentAttribute : " << attribute.parentAttribute << ",";
        if (attribute.parentAttribute.isNull())
            dbg.nospace() << QString(attribute.name.size() + 2 - 2, ' ').toStdString().c_str() << " // default";
        dbg.nospace() << endl;

        // discretizations
        dbg.nospace() << prefix << "  discretizations : ";
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

    QDebug constraintHashHelper(QDebug dbg, const QHash<Analytics::ItemConstraintType, QSet<Analytics::ItemName> > & hash) {
        if (!hash[Analytics::CONSTRAINT_POSITIVE_MATCH_ANY].isEmpty()) {
            dbg.nospace() << "      positive : ";
            constraintItemsHelper(dbg, hash[Analytics::CONSTRAINT_POSITIVE_MATCH_ANY]);
        }
        if (!hash[Analytics::CONSTRAINT_NEGATIVE_MATCH_ANY].isEmpty()) {
            dbg.nospace() << "      negative : ";
            constraintItemsHelper(dbg, hash[Analytics::CONSTRAINT_NEGATIVE_MATCH_ANY]);
        }
        return dbg.nospace();
    }

    QDebug constraintItemsHelper(QDebug dbg, const QSet<Analytics::ItemName> & set) {
        bool firstPrinted = false;

        dbg.nospace() << "[";
        foreach (const Analytics::ItemName & item, set) {
            if (firstPrinted)
                dbg.nospace() << ", ";
            else
                firstPrinted = true;
            dbg.nospace() << item;
        }
        dbg.nospace() << "]," << endl;

        return dbg.nospace();
    }
#endif

}
