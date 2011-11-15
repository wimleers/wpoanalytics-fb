#include "TTWDefinition.h"

namespace Analytics {

    TTWDefinition::TTWDefinition(QMap<char, uint> granularities, QList<char> order) {
        this->reset();
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
               && def1.granularityChar == def2.granularityChar;
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


    //--------------------------------------------------------------------------
    // (De)serialization helper methods.

    QString TTWDefinition::serialize() const {
        QString output;
        for (Granularity g = 0; g < this->numGranularities; g++)
            for (Bucket b = 0; b < this->bucketCount[g]; b++)
                output += this->granularityChar[g];
        return output;
    }

    bool TTWDefinition::deserialize(QString serialized) {
        Granularity g = -1;
        char c, charForGranularity = ' ';

        this->reset();
        this->numBuckets = serialized.size();
        for (int i = 0; i < serialized.size(); i++) {
            c = serialized.at(i).toAscii();

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
