#ifndef TESTTTWDEFINITION_H
#define TESTTTWDEFINITION_H

#include <QtTest/QtTest>
#include "../TTWDefinition.h"

using namespace Analytics;

class TestTTWDefinition : public QObject {
    Q_OBJECT

private slots:
    void serialization();
};


#endif // TESTTTWDEFINITION_H
