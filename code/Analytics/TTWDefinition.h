#ifndef TTWDEFINITION
#define TTWDEFINITION

#include <QString>
#include <QMap>

#include "Item.h"

namespace Analytics {
    typedef int Granularity;
    typedef int Bucket;

    class TTWDefinition {
    public:
        TTWDefinition() {}
        TTWDefinition(QMap<char, uint> granularities, QList<char> order);

        // Operators.
        friend bool operator==(const TTWDefinition & def1, const TTWDefinition & def2);

        // Queries.
        bool bucketIsBeforeGranularity(Bucket b, Granularity g) const;
        Granularity findLowestGranularityAfterBucket(Bucket b) const;

        // (De)serialization helper methods.
        QString serialize() const;
        bool deserialize(QString serialized);

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
