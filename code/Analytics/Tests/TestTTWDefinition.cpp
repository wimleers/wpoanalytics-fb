#include "TestTTWDefinition.h"

void TestTTWDefinition::serialization() {
    QMap<char, uint> granularitiesDefault;
    granularitiesDefault.insert('Q', 4);
    granularitiesDefault.insert('H', 24);
    granularitiesDefault.insert('D', 31);
    granularitiesDefault.insert('M', 12);
    granularitiesDefault.insert('Y', 1);
    TTWDefinition defaultTTWDef(granularitiesDefault,
                             QList<char>() << 'Q' << 'H' << 'D' << 'M' << 'Y');

    QMap<char, uint> granularitiesThirtyDays;
    granularitiesThirtyDays.insert('H', 24);
    granularitiesThirtyDays.insert('D', 30);
    TTWDefinition thirtyDaysTTWDef(granularitiesThirtyDays,
                                   QList<char>() << 'H' << 'D');

    // Serialize both.
    QString defaultTTWDefSerialized = defaultTTWDef.serialize();
    QString thirtyDaysTTWDefSerialized = thirtyDaysTTWDef.serialize();

    // Verify serialization.
    QCOMPARE(defaultTTWDefSerialized, QString("QQQQHHHHHHHHHHHHHHHHHHHHHHHHDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDMMMMMMMMMMMMY"));
    QCOMPARE(thirtyDaysTTWDefSerialized, QString("HHHHHHHHHHHHHHHHHHHHHHHHDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"));

    // Verify deserialization == serialization.
    TTWDefinition deserializedTTWDef;
    deserializedTTWDef.deserialize(defaultTTWDefSerialized);
    QCOMPARE(deserializedTTWDef, defaultTTWDef);
    deserializedTTWDef.deserialize(thirtyDaysTTWDefSerialized);
    QCOMPARE(deserializedTTWDef, thirtyDaysTTWDef);
}
