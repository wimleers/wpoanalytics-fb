#ifndef TESTRULEMINER_H
#define TESTRULEMINER_H

#include <QtTest/QtTest>
#include <QFile>
#include "../FPGrowth.h"

#define protected public
#define private   public
#include "../Ruleminer.h"
#undef protected
#undef private


using namespace Analytics;

class TestRuleMiner : public QObject {
    Q_OBJECT

private slots:
//    void initTestCase() {}
//    void cleanupTestCase() {}
//    void init();
//    void cleanup();
    void basic();
	void generateCandidateItemsets();
};

#endif // TESTRULEMINER_H
