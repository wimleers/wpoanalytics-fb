#ifndef TILTEDTIMEWINDOW_H
#define TILTEDTIMEWINDOW_H

#include <QList>
#include <QVector>
#include <QVariant>

#include <QDebug>

#include "Item.h"


namespace Analytics {

    typedef int Granularity;
    typedef int Bucket;

    class TTWDefinition {
    public:
        TTWDefinition() {}
        TTWDefinition(QMap<char, uint> granularities, QList<char> order) {
            this->numGranularities = granularities.size();
            this->numBuckets = 0;
            foreach (char c, order) {
                uint s = granularities[c];
                this->bucketCount.append(s);
                this->bucketOffset.append(this->numBuckets);
                this->granularityChar.append(c);
                this->numBuckets += s;
            }
        }

        bool bucketIsBeforeGranularity(Bucket b, Granularity g) const {
            Bucket offset = this->bucketOffset[g];
            Bucket count  = this->bucketCount[g];
            return (b <= offset && b <= offset + count);
        }

        Granularity findLowestGranularityAfterBucket(Bucket b) const {
            for (Granularity g = 0; g < this->numGranularities; g++)
                if (this->bucketIsBeforeGranularity(b, g))
                    return g;
            return -1;
        }

        int numGranularities;
        int numBuckets;
        QVector<Bucket> bucketCount;
        QVector<Bucket> bucketOffset;
        QVector<char> granularityChar;
    };

    /*
    QMap<char, uint> granularitiesDefault;
    granularitiesDefault.insert('Q', 4);
    granularitiesDefault.insert('H', 24);
    granularitiesDefault.insert('D', 31);
    granularitiesDefault.insert('M', 12);
    granularitiesDefault.insert('Y', 1);
    TTWDefinition defaultTTWDefinition(granularitiesDefault,
                                       QList<char>() << 'Q' << 'H' << 'D' << 'M' << 'Y');

    QMap<char, uint> granularitiesThirtyDay;
    granularitiesThirtyDay.insert('H', 24);
    granularitiesThirtyDay.insert('D', 30);
    TTWDefinition thirtyDayDefinition(granularitiesThirtyDay,
                                       QList<char>() << 'H' << 'D');
                                       */

    #define TTW_BUCKET_UNUSED MAX_SUPPORT
    #define TTW_EMPTY -1


    class TiltedTimeWindow {
    public:
        TiltedTimeWindow();
        ~TiltedTimeWindow();
        void build(const TTWDefinition & def);

        // Getters.
        uint getCapacityUsed(Granularity g) const { return this->capacityUsed[g]; }
        const TTWDefinition & getDefinition() const { return this->def; }
        quint32 getLastUpdate() const { return this->lastUpdate; }
        Bucket getOldestBucketFilled() const { return this->oldestBucketFilled; }
        SupportCount getSupportForRange(Bucket from, Bucket to) const;

        // Queries.
        bool isEmpty() const { return this->oldestBucketFilled == TTW_EMPTY; }
        Granularity findLowestGranularityAfterBucket(Bucket bucket) const;

        // Setters.
        void append(SupportCount s, quint32 updateID);
        void dropTail(Granularity start);

        // (De)serialization helper methods.
        QVariantMap toVariantMap() const;
        bool fromVariantMap(const QVariantMap & json);

        // Unit testing helper method.
        QVector<SupportCount> getBuckets(int numBuckets) const;

        // Properties.
        SupportCount * buckets;

        // Static methods.
        static Bucket quarterDistanceToBucket(const TTWDefinition & ttwDef, Bucket bucket, bool includeBucketItself);

    protected:
        // Methods.
        void reset(Granularity granularity);
        void shift(Granularity granularity);
        void store(Granularity granularity, SupportCount supportCount);

        // Properties.
        TTWDefinition def;
        bool built;
        Bucket * capacityUsed;
        quint32 lastUpdate;
        Bucket oldestBucketFilled;
    };

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const TTWDefinition & def);
    QDebug operator<<(QDebug dbg, const TiltedTimeWindow & ttw);
#endif
}

#endif // TILTEDTIMEWINDOW_H
