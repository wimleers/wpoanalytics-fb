#include "TTWDefinition.h"

namespace Analytics {

    /**
     * TTWDefinition constructor.
     *
     * @param secPerWindow
     *   Seconds per window, i.e. the number of seconds that the smallest
     *   granularity covers.
     * @param granularities
     *   A map of (char, uint) pairs that describe the single-char identifier
     *   for each granularity as well as how many buckets exist in it.
     * @param order
     *   The order in which the granularities should be used (and stored in
     *   memory), from smallest to largest.
     */
    TTWDefinition::TTWDefinition(uint secPerWindow, QMap<char, uint> granularities, QList<char> order) {
        this->reset();
        this->secPerWindow = secPerWindow;
        this->numGranularities = granularities.size();
        foreach (char c, order) {
            uint s = granularities[c];
            this->bucketCount.append(s);
            this->bucketOffset.append(this->numBuckets);
            this->granularityChar.append(c);
            this->numBuckets += s;
        }
    }


    //--------------------------------------------------------------------------
    // Operators.

    bool operator==(const TTWDefinition & def1, const TTWDefinition & def2) {
        return def1.numGranularities == def2.numGranularities
               && def1.numBuckets == def2.numBuckets
               && def1.bucketCount == def2.bucketCount
               && def1.bucketOffset == def2.bucketOffset
               && def1.granularityChar == def2.granularityChar
               && def1.secPerWindow == def2.secPerWindow;
    }


    //--------------------------------------------------------------------------
    // Queries.

    bool TTWDefinition::bucketIsBeforeGranularity(Bucket b, Granularity g) const {
        Bucket offset = this->bucketOffset[g];
        Bucket count  = this->bucketCount[g];
        return (b <= offset && b <= offset + count);
    }

    Granularity TTWDefinition::findLowestGranularityAfterBucket(Bucket b) const {
        for (Granularity g = 0; g < this->numGranularities; g++)
            if (this->bucketIsBeforeGranularity(b, g))
                return g;
        return -1;
    }

    /**
     * Measure the distance from bucket 0 to the given bucket.
     */
    uint TTWDefinition::calculateSecondsToBucket(Bucket bucket, bool includeBucketItself) const {
        uint secs = 0;
        Granularity g = 0;
        uint secsPerBucket = this->secPerWindow;
        for (Bucket b = 0; b < bucket || (includeBucketItself && b == bucket); b++) {
            // Increase the number of seconds per bucket each time we enter a
            // larger granularity.
            if (g < this->numGranularities - 1 && b >= this->bucketOffset[g + 1]) {
                secsPerBucket *= this->bucketCount[g];
                g++;
            }

            secs += secsPerBucket;
        }

        return secs;
    }

    uint TTWDefinition::calculateTimeOfNextBucket(uint time) const {
        return time - (time % this->secPerWindow) + this->secPerWindow;
    }


    //--------------------------------------------------------------------------
    // (De)serialization helper methods.

    QString TTWDefinition::serialize() const {
        QString output;
        output += QString::number(this->secPerWindow) + ":";
        for (Granularity g = 0; g < this->numGranularities; g++)
            for (Bucket b = 0; b < this->bucketCount[g]; b++)
                output += this->granularityChar[g];
        return output;
    }

    bool TTWDefinition::deserialize(QString serialized) {
        Granularity g = -1;
        char c, charForGranularity = ' ';

        this->reset();

        QStringList parts = serialized.split(":");
        if (parts.size() != 2)
            return false;

        // Part 1: secPerWindow.
        this->secPerWindow = parts[0].toUInt();

        // Part 2: everything else (granularities, buckets).
        this->numBuckets = parts[1].size();
        for (int i = 0; i < parts[1].size(); i++) {
            c = parts[1].at(i).toAscii();

            // Detect new granularities.
            if (c != charForGranularity) {
                charForGranularity = c;
                g++;

                this->numGranularities++;

                // These .append()s will trigger the creation of index g.
                this->bucketCount.append(0);
                this->bucketOffset.append(i);
                this->granularityChar.append(charForGranularity);
            }

            this->bucketCount[g]++;
        }
        return true;
    }


    //--------------------------------------------------------------------------
    // Protected methods.

    void TTWDefinition::reset() {
        this->secPerWindow = 0;
        this->numBuckets = 0;
        this->numGranularities = 0;
        this->bucketCount.clear();
        this->bucketOffset.clear();
        this->granularityChar.clear();
    }


    //--------------------------------------------------------------------------
    // Debugging.

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const TTWDefinition & def) {
        static const char * prefix = "  ";
        dbg.nospace() << "{" << endl;
        // Line 0: number of seconds in the smallest granularity.
        dbg.nospace() << prefix << def.secPerWindow << "s per " << def.granularityChar[0] << endl;
        // Line 1: how many buckets in each granularity.
        dbg.nospace() << prefix;
        for (int i = 0; i < def.numGranularities; i++) {
            QString s;
            s.sprintf("[ %5d*%-5c ]", def.bucketCount[i], def.granularityChar[i]);
            dbg.nospace() << s.toStdString().c_str();
        }
        dbg.nospace() << endl;
        // Line 2: visualize the buckets.
        dbg.nospace() << prefix;
        for (int i = 0; i < def.numGranularities; i++) {
            QString s;
            s.sprintf("[ %c ][...][ %c ]", def.granularityChar[i], def.granularityChar[i]);
            dbg.nospace() << s.toStdString().c_str();
        }
        dbg.nospace() << endl;
        // Line 3: show the indices of the first and last bucket of each granularity.
        dbg.nospace() << prefix;
        for (int i = 0; i < def.numGranularities; i++) {
            QString s;
            s.sprintf("[ %-2d  ...   %-2d]", def.bucketOffset[i], def.bucketOffset[i] + def.bucketCount[i] - 1);
            dbg.nospace() << s.toStdString().c_str();
        }
        dbg.nospace() << endl;
        dbg.nospace() << "}" << endl;
        return dbg.nospace();
    }

#endif
}
