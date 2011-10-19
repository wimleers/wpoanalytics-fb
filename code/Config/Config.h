#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QList>
#include <QHash>
#include <QMap>
#include <QMap>
#include <QSet>
#include <QMutableSetIterator>
#include <QFile>
#include <QDebug>

#include "qxtjson.h"

#include "typedefs.h"
#include "../Analytics/Item.h"
#include "../Analytics/Constraints.h"

namespace Config {

    struct Discretization {
        bool isDefault() const { return this->circumstances.isEmpty(); }

        QMap<EpisodeDuration, EpisodeSpeed> thresholds;
        Circumstances circumstances;
    };

    struct Attribute {
        Attribute(const QString & field ="",
                  const QString & name  = QString::null,
                  bool isEpisode        = false,
                  QSet<Discretization> discretizations = QSet<Discretization>())
            : field(field), name(name), isEpisode(isEpisode), discretizations(discretizations) {}

        QString field;  // required
        QString name;   // optional, defaults to QString::null
        bool isEpisode; // optional, defaults to false
        QString parentAttribute; // optional, defaults to QString::null
        QSet<Discretization> discretizations; // optional, default to empty hash
                                               // if not empty, then at least one discretization must have *no* circumstances, this will be the default
    };
    uint qHash(const Discretization & d);
    inline bool operator==(const Discretization & d1, const Discretization & d2) {
        return d1.thresholds == d2.thresholds && d1.circumstances == d2.circumstances;
    }
    inline bool operator!=(const Discretization & d1, const Discretization & d2) {
        return !(d1 == d2);
    }


    class Config {
    public:
        Config();
        bool parse(const QString & fileName);

        // Getters (query: patterns).
        double getMinPatternSupport() const { return this->minPatternSupport; }
        double getMinPotentialPatternSupport() const { return this->minPotentialPatternSupport; }
        const Analytics::ItemConstraintsHash & getPatternItemConstraints() const { return this->patternItemConstraints; }

        // Getters (query: association rules).
        double getMinRuleConfidence() const { return this->minRuleConfidence; }
        const Analytics::ItemConstraintsHash & getRuleConsequentItemConstraints() const { return this->ruleConsequentItemConstraints; }

        // Getters (attributes).
        const QHash<EpisodeName, Attribute> & getCategoricalAttributes() const { return this->categoricalAttributes; }
        const QHash<EpisodeName, Attribute> & getNumericalAttributes() const { return this->numericalAttributes; }

        // Utility methods.
        EpisodeSpeed discretize(const EpisodeName & episodeName,
                                        EpisodeDuration duration,
                                        Circumstances circumstances
                                        ) const;

    protected:
        // Parsing helper methods (all static).
        static double parseDouble(const QVariantMap & json, const QString & key);
        static Analytics::ItemConstraintsHash parseConstraints(const QVariantMap & json, const QString & key);
        static Attribute parseAttribute(const QVariantMap & json, const QString & field);
        static QSet<Discretization> parseDiscretizations(const QVariant & json, const QString & field);

        // Query: patterns
        double minPatternSupport;
        double minPotentialPatternSupport;
        Analytics::ItemConstraintsHash patternItemConstraints;

        // Query: association rules
        double minRuleConfidence;
        Analytics::ItemConstraintsHash ruleConsequentItemConstraints;

        // Attributes
        QHash<EpisodeName, Attribute> categoricalAttributes;
        QHash<EpisodeName, Attribute> numericalAttributes;

#ifdef DEBUG
    friend QDebug operator<<(QDebug dbg, const Config & config);
#endif
    };

#ifdef DEBUG
    // QDebug() streaming output operators.
    QDebug operator<<(QDebug dbg, const Attribute & attribute);
    QDebug operator<<(QDebug dbg, const QSet<Discretization> & discretizations);
    QDebug operator<<(QDebug dbg, const Discretization & discretization);
    QDebug operator<<(QDebug dbg, const Config & config);
    QDebug attributeHelper(QDebug dbg, const Attribute & attribute, const char * prefix = "");
    QDebug discretizationsHelper(QDebug debg, const QSet<Discretization> & discretizations, const char * prefix = "");
    QDebug constraintHashHelper(QDebug dbg, const Analytics::ItemConstraintsHash & hash);
    QDebug constraintItemsHelper(QDebug dbg, const QSet<Analytics::ItemName> & set);
#endif

}


#endif // CONFIG_H
