#include "TestConstraints.h"

void TestConstraints::basic_data() {
    QTest::addColumn<QSet<ItemName> >("positive_constraint_1");
    QTest::addColumn<QSet<ItemName> >("positive_constraint_2");
    QTest::addColumn<QSet<ItemName> >("negative_constraint_1");
    QTest::addColumn<QSet<ItemName> >("negative_constraint_2");
    QTest::addColumn<bool>("result_1");
    QTest::addColumn<bool>("result_2");
    QTest::addColumn<bool>("result_3");
    QTest::addColumn<bool>("result_4");
    QTest::addColumn<bool>("result_5");

    // Simple positive.
    QTest::newRow("simple positive constraint")
            << (QSet<ItemName>() << "A")
            << QSet<ItemName>()
            << QSet<ItemName>()
            << QSet<ItemName>()
            << true << true << false << true << true;

    QTest::newRow("slightly more elaborate positive constraint that shows the ORing")
            << (QSet<ItemName>() << "B" << "C")
            << QSet<ItemName>()
            << QSet<ItemName>()
            << QSet<ItemName>()
            << false << true << true << true << false;

    QTest::newRow("simple positive constraint, with no matches")
            << (QSet<ItemName>() << "E")
            << QSet<ItemName>()
            << QSet<ItemName>()
            << QSet<ItemName>()
            << false << false << false << false << false;

    // Simple negative.
    QTest::newRow("simple negative constraint, with no matches")
            << QSet<ItemName>()
            << QSet<ItemName>()
            << (QSet<ItemName>() << "A")
            << QSet<ItemName>()
            << false << false << true << false << false;

    QTest::newRow("slightly more elaborate negative constraint that shows the ORing")
            << QSet<ItemName>()
            << QSet<ItemName>()
            << (QSet<ItemName>() << "B" << "D")
            << QSet<ItemName>()
            << true << false << false << false << false;

    QTest::newRow("slightly negative constraint with no matches")
            << QSet<ItemName>()
            << QSet<ItemName>()
            << (QSet<ItemName>() << "E")
            << QSet<ItemName>()
            << true << true << true << true << true;

    // Advanced positive: positive constraints AND'ed together.
    QTest::newRow("advanced positive: accepts supersets of (A,B), (A,C), (A,B,C)")
            << (QSet<ItemName>() << "B" << "C")
            << (QSet<ItemName>() << "A")
            << QSet<ItemName>()
            << QSet<ItemName>()
            << false << true << false << true << false;

    // Advanced negative.
    QTest::newRow("advanced negative: accepts no supersets of (A), (B), (C), or any combination thereof")
            << QSet<ItemName>()
            << QSet<ItemName>()
            << (QSet<ItemName>() << "B" << "C")
            << (QSet<ItemName>() << "A")
            << false << false << false << false << false;

    QTest::newRow("advanced negative: although defined differently, this should behave the same as the 'slightly more elaborative negative constraint'")
            << QSet<ItemName>()
            << QSet<ItemName>()
            << (QSet<ItemName>() << "B")
            << (QSet<ItemName>() << "D")
            << true << false << false << false << false;

    // Advanced both.
    QTest::newRow("advanced both: accepts supersets of (A,B), (A,C), (A,B,C), except if they also are a superset of (D)")
            << (QSet<ItemName>() << "B" << "C")
            << (QSet<ItemName>() << "A")
            << (QSet<ItemName>() << "D")
            << QSet<ItemName>()
            << false << true << false << false << false;
}

void TestConstraints::alternate_data() {
    return this->basic_data();
}

void TestConstraints::alternateWildcard_data() {
    QTest::addColumn<QSet<ItemName> >("positive_constraint_1");
    QTest::addColumn<QSet<ItemName> >("negative_constraint_1");
    QTest::addColumn<bool>("result_1");
    QTest::addColumn<bool>("result_2");
    QTest::addColumn<bool>("result_3");

    // Positive.
    QTest::newRow("positive foo*")
            << (QSet<ItemName>() << "foo*")
            << QSet<ItemName>()
            << true << true << false;

    QTest::newRow("positive foo:*")
            << (QSet<ItemName>() << "foo:*")
            << QSet<ItemName>()
            << false << true << false;

    QTest::newRow("positive *foo:*")
            << (QSet<ItemName>() << "*foo:*")
            << QSet<ItemName>()
            << false << true << true;

    // Negative.
    QTest::newRow("negative foo*")
            << QSet<ItemName>()
            << (QSet<ItemName>() << "foo*")
            << false << false << true;

    QTest::newRow("negative foo:*")
            << QSet<ItemName>()
            << (QSet<ItemName>() << "foo:*")
            << true << false << true;

    QTest::newRow("negative *foo:*")
            << QSet<ItemName>()
            << (QSet<ItemName>() << "*foo:*")
            << true << false << false;

    // Both.
    QTest::newRow("positive *o:*, negative *:*:*")
            << (QSet<ItemName>() << "*o:*")
            << (QSet<ItemName>() << "*:*:*")
            << false << true << false;
}

void TestConstraints::basic() {
    // Build ItemIDNameHash.
    ItemIDNameHash itemIDNameHash;
    itemIDNameHash.insert(1, "A");
    itemIDNameHash.insert(2, "B");
    itemIDNameHash.insert(3, "C");
    itemIDNameHash.insert(4, "D");
    itemIDNameHash.insert(5, "E");
    itemIDNameHash.insert(6, "F");
    itemIDNameHash.insert(7, "G");

    // Create a few frequent itemsets.
    ItemIDList f1, f2, f3, f4, f5;
    f1 << 1;
    f2 << 1 << 2;
    f3 << 2 << 4;
    f4 << 1 << 3 << 4;
    f5 << 4 << 1;

    // Retrieve expected values.
    QFETCH(QSet<ItemName>, positive_constraint_1);
    QFETCH(QSet<ItemName>, positive_constraint_2);
    QFETCH(QSet<ItemName>, negative_constraint_1);
    QFETCH(QSet<ItemName>, negative_constraint_2);
    QFETCH(bool, result_1);
    QFETCH(bool, result_2);
    QFETCH(bool, result_3);
    QFETCH(bool, result_4);
    QFETCH(bool, result_5);

    // Compare the results with the expected values.
    Constraints c;
    c.addItemConstraint(positive_constraint_1, ItemConstraintPositive);
    c.addItemConstraint(positive_constraint_2, ItemConstraintPositive);
    c.addItemConstraint(negative_constraint_1, ItemConstraintNegative);
    c.addItemConstraint(negative_constraint_2, ItemConstraintNegative);
    c.preprocessItemIDNameHash(itemIDNameHash);
    //qDebug() << c;
    QCOMPARE(c.matchItemset(f1), result_1);
    QCOMPARE(c.matchItemset(f2), result_2);
    QCOMPARE(c.matchItemset(f3), result_3);
    QCOMPARE(c.matchItemset(f4), result_4);
    QCOMPARE(c.matchItemset(f5), result_5);
}

void TestConstraints::alternate() {
    // Create a few frequent itemsets.
    QSet<ItemName> f1, f2, f3, f4, f5;
    f1 << "A";
    f2 << "A" << "B";
    f3 << "B" << "D";
    f4 << "A" << "C" << "D";
    f5 << "D" << "A";

    // Retrieve expected values.
    QFETCH(QSet<ItemName>, positive_constraint_1);
    QFETCH(QSet<ItemName>, positive_constraint_2);
    QFETCH(QSet<ItemName>, negative_constraint_1);
    QFETCH(QSet<ItemName>, negative_constraint_2);
    QFETCH(bool, result_1);
    QFETCH(bool, result_2);
    QFETCH(bool, result_3);
    QFETCH(bool, result_4);
    QFETCH(bool, result_5);

    // Compare the results with the expected values.
    Constraints c;
    c.addItemConstraint(positive_constraint_1, ItemConstraintPositive);
    c.addItemConstraint(positive_constraint_2, ItemConstraintPositive);
    c.addItemConstraint(negative_constraint_1, ItemConstraintNegative);
    c.addItemConstraint(negative_constraint_2, ItemConstraintNegative);
    //qDebug() << c;
    QCOMPARE(c.matchItemset(f1), result_1);
    QCOMPARE(c.matchItemset(f2), result_2);
    QCOMPARE(c.matchItemset(f3), result_3);
    QCOMPARE(c.matchItemset(f4), result_4);
    QCOMPARE(c.matchItemset(f5), result_5);
}

void TestConstraints::alternateWildcard() {
    // Create a few frequent itemsets.
    QSet<ItemName> f1, f2, f3, f4;
    f1 << "foo";
    f2 << "foo:bar";
    f3 << "baz:foo:bar";

    // Retrieve expected values.
    QFETCH(QSet<ItemName>, positive_constraint_1);
    QFETCH(QSet<ItemName>, negative_constraint_1);
    QFETCH(bool, result_1);
    QFETCH(bool, result_2);
    QFETCH(bool, result_3);

    // Compare the results with the expected values.
    Constraints c;
    c.addItemConstraint(positive_constraint_1, ItemConstraintPositive);
    c.addItemConstraint(negative_constraint_1, ItemConstraintNegative);
    //qDebug() << c;
    QCOMPARE(c.matchItemset(f1), result_1);
    QCOMPARE(c.matchItemset(f2), result_2);
    QCOMPARE(c.matchItemset(f3), result_3);
}

void TestConstraints::edgeCases() {
    // Calling removeItem() for a preprocessed item when not all item
    // constraints have been preprocessed. This should not break.
    Constraints c;
    c.addItemConstraint(QSet<ItemName>() << "foo", ItemConstraintPositive);
    c.addItemConstraint(QSet<ItemName>() << "bar", ItemConstraintPositive);
    c.addItemConstraint(QSet<ItemName>() << "baz", ItemConstraintPositive);
    c.preprocessItem("foo", 12345);
    c.preprocessItem("bar", 98765);
    c.removeItem(98765);
}
