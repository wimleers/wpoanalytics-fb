#ifndef TILTEDTIMEWINDOW_H
#define TILTEDTIMEWINDOW_H

#include <QList>
#include <QVector>
#include <QVariant>

#include <QDebug>

#include "Item.h"
#include "TTWDefinition.h"


namespace Analytics {

    #define TTW_BUCKET_UNUSED MAX_SUPPORT
    #define TTW_EMPTY -1


    class TiltedTimeWindow {
    public:
        TiltedTimeWindow();
        ~TiltedTimeWindow();
        void build(const TTWDefinition & def, bool rebuild = false);

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

    protected:
        // Methods.
        void reset(Granularity granularity);
        void shift(Granularity granularity);
        void store(Granularity granularity, SupportCount supportCount);

        void clear();

        // Properties.
        TTWDefinition def;
        bool built;
        Bucket * capacityUsed;
        quint32 lastUpdate;
        Bucket oldestBucketFilled;
    };

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const TiltedTimeWindow & ttw);
#endif
}

#endif // TILTEDTIMEWINDOW_H
