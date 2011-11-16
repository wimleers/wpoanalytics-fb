#include "TestTTWDefinition.h"

void TestTTWDefinition::serialization() {
    QMap<char, uint> granularitiesDefault;
    granularitiesDefault.insert('Q', 4);
    granularitiesDefault.insert('H', 24);
    granularitiesDefault.insert('D', 31);
    granularitiesDefault.insert('M', 12);
    granularitiesDefault.insert('Y', 1);
    TTWDefinition defaultTTWDef(900,
                                granularitiesDefault,
                                QList<char>() << 'Q' << 'H' << 'D' << 'M' << 'Y');

    QMap<char, uint> granularitiesThirtyDays;
    granularitiesThirtyDays.insert('H', 24);
    granularitiesThirtyDays.insert('D', 30);
    TTWDefinition thirtyDaysTTWDef(3600,
                                   granularitiesThirtyDays,
                                   QList<char>() << 'H' << 'D');

    // Serialize both.
    QString defaultTTWDefSerialized = defaultTTWDef.serialize();
    QString thirtyDaysTTWDefSerialized = thirtyDaysTTWDef.serialize();

    // Verify serialization.
    QCOMPARE(defaultTTWDefSerialized, QString("900:QQQQHHHHHHHHHHHHHHHHHHHHHHHHDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDMMMMMMMMMMMMY"));
    QCOMPARE(thirtyDaysTTWDefSerialized, QString("3600:HHHHHHHHHHHHHHHHHHHHHHHHDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"));

    // Verify deserialization == serialization.
    TTWDefinition deserializedTTWDef;
    deserializedTTWDef.deserialize(defaultTTWDefSerialized);
    QCOMPARE(deserializedTTWDef, defaultTTWDef);
    deserializedTTWDef.deserialize(thirtyDaysTTWDefSerialized);
    QCOMPARE(deserializedTTWDef, thirtyDaysTTWDef);
}
