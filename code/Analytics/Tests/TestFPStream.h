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
};

#endif // TESTFPSTREAM_H
