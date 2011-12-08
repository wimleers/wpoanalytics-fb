#ifndef TTWDEFINITION
#define TTWDEFINITION

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include <QDebug>

#include "../common/common.h"

namespace Analytics {
    class TTWDefinition {
    public:
        TTWDefinition() {}
        TTWDefinition(uint secPerWindow, QMap<char, uint> granularities, QList<char> order);

        // Operators.
        friend bool operator==(const TTWDefinition & def1, const TTWDefinition & def2);
        friend bool operator!=(const TTWDefinition & def1, const TTWDefinition & def2);

        // Getters.
        uint getSecPerWindow() const { return this->secPerWindow; }

        // Queries.
        bool bucketIsBeforeGranularity(Bucket b, Granularity g) const;
        Granularity findLowestGranularityAfterBucket(Bucket b) const;
        uint calculateSecondsToBucket(Bucket bucket, bool includeBucketItself) const;
        uint calculateTimeOfNextBucket(uint time) const;

        // (De)serialization helper methods.
        QString serialize() const;
        bool deserialize(QString serialized);

        uint secPerWindow;
        int numGranularities;
        int numBuckets;
        QVector<Bucket> bucketCount;
        QVector<Bucket> bucketOffset;
        QVector<char> granularityChar;

    protected:
        void reset();
    };

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const TTWDefinition & def);
#endif
}

#endif // TTWDEFINITION
