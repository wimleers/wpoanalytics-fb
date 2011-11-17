#include "TiltedTimeWindow.h"

namespace Analytics {

    //--------------------------------------------------------------------------
    // Public methods.

    TiltedTimeWindow::TiltedTimeWindow() {
        // Arrays that still need to be allocated. @see build().
        this->buckets = NULL;
        this->capacityUsed = NULL;

        this->clear();
    }

    void TiltedTimeWindow::clear() {
        this->oldestBucketFilled = TTW_EMPTY;
        this->lastUpdate = 0;
        this->built = false;
        if (this->built) {
            delete [] this->buckets;
            delete [] this->capacityUsed;
        }
    }

    TiltedTimeWindow::~TiltedTimeWindow() {
        this->clear();
    }

    void TiltedTimeWindow::build(const TTWDefinition & def, bool rebuild) {
        if (this->built && !rebuild)
            return;
        else if (this->built)
            this->clear();

        this->def = def;

        // Allocate the arrays.
        this->buckets = new SupportCount[this->def.numBuckets];
        this->capacityUsed = new Bucket[this->def.numGranularities];

        // Fill with default values.
        memset(this->buckets, TTW_BUCKET_UNUSED, this->def.numBuckets * sizeof(SupportCount));
        memset(this->capacityUsed, 0, this->def.numGranularities * sizeof(Bucket));

        this->built = true;
    }

    /**
     * Append a quarter for the given update ID.
     *
     * @param supportCount
     *   The support for this quarter.
     * @param updateID
     *   The updateID that corresponds with this quarter. Should start at 1.
     *   When it equals the last used updateID of this TiltedTimeWindow, the
     *   given supportCount will simply be added to the supportCount in the last
     *   bucket to which was appended (which is always the same one). When it
     *   equals 0, a new quarter is always created (this can be used to sync
     *   TiltedTimeWindows).
     */
    void TiltedTimeWindow::append(SupportCount supportCount, quint32 updateID) {
        Q_ASSERT(this->built);

        if (updateID != this->lastUpdate || updateID == 0) {
            if (updateID != 0)
                this->lastUpdate = updateID;
            this->store((Granularity) 0, supportCount);
        }
        else {
            // Allow adding to the last bucket that was created. This is always
            // bucket 0.
            this->buckets[0] += supportCount;
        }
    }

    /**
     * Drop the tail. Only allow entire granularities to be dropped. Otherwise
     * tail dropping/pruning may result in out-of-sync TiltedTimeWindows. This
     * of course leads to TiltedTimeWindows tipping over to the higher-level
     * granularities at different points in time, which would cause incorrect
     * results.
     *
     * @param start
     *   The granularity starting from which all buckets should be dropped.
     */
    void TiltedTimeWindow::dropTail(Granularity start) {
        Q_ASSERT(this->built);

        // Find the granularity to which it belongs and reset every
        // granularity along the way.
        Granularity g;
        for (g = this->def.numGranularities - 1; g >= start; g--)
            this->reset(g);
    }

    /**
     * Get the support in this TiltedTimeWindow for a range of buckets.
     *
     * @param from
     *   The range starts at this bucket.
     * @param to
     *   The range ends at this bucket.
     * @return
     *   The total support in the buckets in the given range.
     */
    SupportCount TiltedTimeWindow::getSupportForRange(Bucket from, Bucket to) const {
        Q_ASSERT(this->built);
        Q_ASSERT(from >= 0);
        Q_ASSERT(to >= 0);
        Q_ASSERT(from <= to);
        Q_ASSERT(from < this->def.numBuckets);
        Q_ASSERT(to   < this->def.numBuckets);

        // Return 0 if this TiltedTimeWindow is empty.
        if (this->isEmpty())
            return 0;

        // Otherwise, count the sum.
        SupportCount sum = 0;
        for (Bucket i = from; i <= to && i <= this->oldestBucketFilled; i++) {
            if (this->buckets[i] != TTW_BUCKET_UNUSED)
                sum += this->buckets[i];
        }

        return sum;
    }

    Granularity TiltedTimeWindow::findLowestGranularityAfterBucket(Bucket bucket) const {
        return this->def.findLowestGranularityAfterBucket(bucket);
    }

    QVariantMap TiltedTimeWindow::toVariantMap() const {
        Q_ASSERT(this->built);

        QVariantMap variantRepresentation;

        QList<QVariant> bucketsVariant;
        if (!this->isEmpty())
            for (Bucket b = 0; b <= this->oldestBucketFilled; b++)
                bucketsVariant << QVariant((int) this->buckets[b]); // TRICKY: typecast to int, uint results in nothing being printed. Probably a bug in QxtJSON.

        variantRepresentation.insert("v", 1); // Version.
        variantRepresentation.insert("buckets", bucketsVariant);
        // "oldestBucketFilled" can be deduced from "buckets".
        // "capacityUsed" can be deduced from "buckets".
        variantRepresentation.insert("lastUpdate", (int) this->lastUpdate); // TRICKY: typecast to int

        return variantRepresentation;
    }

    bool TiltedTimeWindow::fromVariantMap(const QVariantMap & json) {
        Q_ASSERT(this->built);

        int version = json["v"].toInt();

        if (version == 1) {
            QList<QVariant> bucketValues = json["buckets"].toList();

            // this->oldestBucketFilled
            if (bucketValues.isEmpty())
                this->oldestBucketFilled = TTW_EMPTY;
            else
                this->oldestBucketFilled = bucketValues.size() - 1;
            // this->buckets
            for (int i = 0; i < bucketValues.size(); i++)
                this->buckets[i] = (SupportCount) bucketValues[i].toInt();
            // this->capacityUsed
            Bucket bucketsRemainingToBeFilled = bucketValues.size();
            for (Granularity g = 0; g < (Granularity) this->def.numGranularities; g++) {
                if (bucketsRemainingToBeFilled < this->def.bucketCount[g]) {
                    this->capacityUsed[g] = bucketsRemainingToBeFilled;
                    bucketsRemainingToBeFilled = 0;
                }
                else {
                    this->capacityUsed[g] = this->def.bucketCount[g];
                    bucketsRemainingToBeFilled -= this->def.bucketCount[g];
                }
            }
            // this->lastUpdate
            this->lastUpdate = (quint32) json["lastUpdate"].toInt();

            return true;
        }

        return false;
    }

    QVector<SupportCount> TiltedTimeWindow::getBuckets(int numBuckets) const {
        Q_ASSERT(this->built);
        Q_ASSERT(numBuckets >= 0 && numBuckets <= this->def.numBuckets);

        QVector<SupportCount> v;
        for (int i = 0; i < numBuckets; i++)
            v.append(this->buckets[i]);
        return v;
    }

    //--------------------------------------------------------------------------
    // Private methods.

    /**
     * Reset a granularity.
     *
     * @param granularity
     *   The granularity that should be reset.
     */
    void TiltedTimeWindow::reset(Granularity granularity) {
        Q_ASSERT(this->built);

        Bucket offset = this->def.bucketOffset[granularity];
        Bucket count = this->def.bucketCount[granularity];

        // Reset this granularity's buckets.
        memset(this->buckets + offset, TTW_BUCKET_UNUSED, count * sizeof(SupportCount));

        // Update this granularity's used capacity.
        this->capacityUsed[granularity] = 0;

        // Update oldestBucketFilled by resetting it to the beginning of this
        // granularity, but only if it is in fact currently pointing to a
        // position *within* this granularity (i.e. if it is in the range
        // [offset, offset + count - 1]).
        if (this->oldestBucketFilled >= offset && this->oldestBucketFilled < offset + count)
            this->oldestBucketFilled = (offset == 0) ? TTW_EMPTY : offset - 1;
    }

    /**
     * Shift the support counts from one granularity to the next.
     *
     * @param granularity
     *   The granularity that should be shifted.
     */
    void TiltedTimeWindow::shift(Granularity granularity) {
        Q_ASSERT(this->built);

        // If the next granularity does not exist, reset this granularity.
        if (granularity + 1 > this->def.numGranularities - 1)
            this->reset(granularity);

        Bucket offset = this->def.bucketOffset[granularity];
        Bucket count  = this->def.bucketCount[granularity];

        // Calculate the sum of this granularity's buckets.
        SupportCount sum = 0;
        for (Bucket b = 0; b < count; b++)
            sum += buckets[offset + b];

        // Reset this granularity.
        this->reset(granularity);

        // Store the sum of the buckets in the next granularity.
        this->store((Granularity) (granularity + 1), sum);
    }

    /**
     * Store a next support count in a granularity.
     *
     * @param granularity
     *   The granularity to which this support count should be appended.
     * @param supportCount
     *   The supportCount that should be appended.
     */
    void TiltedTimeWindow::store(Granularity granularity, SupportCount supportCount) {
        Q_ASSERT(this->built);

        Bucket offset       = this->def.bucketOffset[granularity];
        Bucket count        = this->def.bucketCount[granularity];
        Bucket capacityUsed = this->capacityUsed[granularity];

        // If the current granularity's maximum capacity has been reached,
        // then shift it to the next (less granular) granularity.
        if (capacityUsed == count) {
            this->shift(granularity);
            capacityUsed = this->capacityUsed[granularity];
        }

        // Store the value (in the first bucket of this granularity, which
        // means we'll have to move the data in previously filled in buckets
        // in this granularity) and update this granularity's capacity.
        if (capacityUsed > 0)
            memmove(buckets + offset + 1, buckets + offset, capacityUsed * sizeof(SupportCount));
        buckets[offset + 0] = supportCount;
        this->capacityUsed[granularity]++;

        // Update oldestbucketFilled.
        if (this->isEmpty() || this->oldestBucketFilled < ((int)offset + this->capacityUsed[granularity] - 1))
            this->oldestBucketFilled = offset + this->capacityUsed[granularity] - 1;
    }


    //--------------------------------------------------------------------------
    // Debugging.

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const TiltedTimeWindow & ttw) {
        const TTWDefinition & def = ttw.getDefinition();
        int capacityUsed, offset;
        QVector<SupportCount> buckets = ttw.getBuckets(def.numBuckets);

        dbg.nospace() << "{";

        for (Granularity g = 0; g < def.numGranularities; g++) {
            capacityUsed = ttw.getCapacityUsed(g);
            if (capacityUsed == 0)
                break;

            dbg.nospace() << def.granularityChar[g] << "={";

            // Print the contents of this granularity.
            offset = def.bucketOffset[g];
            for (int b = 0; b < capacityUsed; b++) {
                if (b > 0)
                    dbg.nospace() << ", ";

                dbg.nospace() << buckets[offset + b];
            }

            dbg.nospace() << "}";
            if (g < def.numGranularities - 1 && ttw.getCapacityUsed(g + 1) > 0)
                dbg.nospace() << ", ";
        }

        dbg.nospace() << "} (lastUpdate=" << ttw.getLastUpdate() << ")";

        return dbg.nospace();
    }
#endif
}
