#ifndef TESTPATTERNTREE_H
#define TESTPATTERNTREE_H

#include <QtTest/QtTest>
#include "../PatternTree.h"

using namespace Analytics;

class TestPatternTree : public QObject {
    Q_OBJECT

private slots:
    void basic();
    void additionsRemainInSync();
    void getFrequentItemsetsForRange();

private:
    PatternTree * buildBasicPatternTree();
    static const TTWDefinition getTTWDefinition() {
        QMap<char, uint> granularitiesDefault;
        granularitiesDefault.insert('Q', 4);
        granularitiesDefault.insert('H', 24);
        granularitiesDefault.insert('D', 31);
        granularitiesDefault.insert('M', 12);
        granularitiesDefault.insert('Y', 1);
        TTWDefinition defaultTTWDefinition(3600,
                                           granularitiesDefault,
                                           QList<char>() << 'Q' << 'H' << 'D' << 'M' << 'Y');
        return defaultTTWDefinition;
    }
};

#endif // TESTPATTERNTREE_H
