#ifndef TESTCONSTRAINTS_H
#define TESTCONSTRAINTS_H

#include <QtTest/QtTest>
#include <QFile>
#include "../Item.h"

// #define protected public
// #define private   public
#include "../Constraints.h"
// #undef protected
// #undef private


using namespace Analytics;

class TestConstraints : public QObject {
    Q_OBJECT

private slots:
//    void initTestCase() {}
//    void cleanupTestCase() {}
//    void init();
//    void cleanup();
    void basic();
    void basic_data();
    void alternate();
    void alternate_data();
    void alternateWildcard();
    void alternateWildcard_data();
    void noConstraints();
    void edgeCases();
};


Q_DECLARE_METATYPE(QSet<ItemName>)

#endif // TESTCONSTRAINTS_H
