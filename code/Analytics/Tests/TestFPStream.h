#ifndef TESTFPSTREAM_H
#define TESTFPSTREAM_H

#include <QtTest/QtTest>
#include <QTemporaryFile>
#include <QTextStream>
#include "../FPStream.h"

using namespace Analytics;

class TestFPStream : public QObject {
    Q_OBJECT

private slots:
    void calculateDroppableTail();
    void basic();
	void serialization();

private:
    void verifyNode(const PatternTree & patternTree,
                    const FPNode<TiltedTimeWindow> * const node,
                    ItemID itemID,
                    unsigned int nodeID,
                    const ItemIDList & referencePattern,
                    const QVector<SupportCount> & referenceBuckets,
                    bool verifyNodeID = true);

    void verifyShapeOfBasicTree(const FPStream * fpstream, bool verifyNodeIDs = true);
    static const TTWDefinition getTTWDefinition() {
        QMap<char, uint> granularitiesDefault;
        granularitiesDefault.insert('Q', 4);
        granularitiesDefault.insert('H', 24);
        granularitiesDefault.insert('D', 31);
        granularitiesDefault.insert('M', 12);
        granularitiesDefault.insert('Y', 1);
        TTWDefinition defaultTTWDefinition(granularitiesDefault,
                                           QList<char>() << 'Q' << 'H' << 'D' << 'M' << 'Y');
        return defaultTTWDefinition;
    }
};

#endif // TESTFPSTREAM_H
