#ifndef TESTTTWDEFINITION_H
#define TESTTTWDEFINITION_H

#include <QtTest/QtTest>
#include "../TTWDefinition.h"

using namespace Analytics;

class TestTTWDefinition : public QObject {
    Q_OBJECT

private slots:
    void serialization();
    void serialization_data();
};

// This typedef only exists because Q_DECLARE_METATYPE cannot handle templates
// with multiple parameters.
typedef QMap<char, uint> MapCharUint;

Q_DECLARE_METATYPE(MapCharUint)
Q_DECLARE_METATYPE(QList<char>)

#endif // TESTTTWDEFINITION_H
