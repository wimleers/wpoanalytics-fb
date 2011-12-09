#include "TestRuleMiner.h"

void TestRuleMiner::basic() {
    QList<QStringList> transactions;
    transactions.append(QStringList() << "A" << "B" << "C" << "D");
    transactions.append(QStringList() << "A" << "B");
    transactions.append(QStringList() << "A" << "C");
    transactions.append(QStringList() << "A" << "B" << "C");
    transactions.append(QStringList() << "A" << "D");
    transactions.append(QStringList() << "A" << "C" << "D");
    transactions.append(QStringList() << "C" << "B");
    transactions.append(QStringList() << "B" << "C");
    transactions.append(QStringList() << "C" << "D");
    transactions.append(QStringList() << "C" << "E");

    Constraints constraints;

    FPNode<SupportCount>::resetLastNodeID();
    ItemIDNameHash itemIDNameHash;
    ItemNameIDHash itemNameIDHash;
    ItemIDList sortedFrequentItemIDs;
    FPGrowth * fpgrowth = new FPGrowth(transactions, 0.4 * transactions.size(), &itemIDNameHash, &itemNameIDHash, &sortedFrequentItemIDs);
    QList<FrequentItemset> frequentItemsets = fpgrowth->mineFrequentItemsets(FPGROWTH_SYNC);

    QList<AssociationRule> associationRules = RuleMiner::mineAssociationRules(frequentItemsets, 0.8, constraints, fpgrowth);

    // Helpful for debugging/expanding this test.
    // Currently, this should match:
    // ({B(1)} => {C(2)} (conf=0.8))
    //qDebug() << associationRules;

    // Verify the results.
    QCOMPARE(associationRules.size(), 1);
    QCOMPARE(associationRules[0].antecedent, (ItemIDList() << 1));
    QCOMPARE(associationRules[0].consequent, (ItemIDList() << 2));
    QCOMPARE(associationRules[0].support,    (SupportCount) 4);
    QCOMPARE(associationRules[0].confidence, (float) 0.8);

    delete fpgrowth;
}

void TestRuleMiner::generateCandidateItemsets() {
    QList<ItemIDList> frequentItemsubsets, result;

    // 1-item consequents.
    frequentItemsubsets.clear();
    frequentItemsubsets << (ItemIDList() << 1);
    frequentItemsubsets << (ItemIDList() << 2);
    frequentItemsubsets << (ItemIDList() << 3);
    result = RuleMiner::generateCandidateItemsets(frequentItemsubsets);
    QCOMPARE(result, QList<ItemIDList>() << (ItemIDList() << 1 << 2)
                                         << (ItemIDList() << 1 << 3)
                                         << (ItemIDList() << 2 << 3));

    // 2-item consequents.
    // It looks like (1,2,3) should be the result, but only (1,2) and (1,3)
    // exist in the source: (2,3) does not exist, hence there are no results.
    frequentItemsubsets.clear();
    frequentItemsubsets << (ItemIDList() << 1 << 2);
    frequentItemsubsets << (ItemIDList() << 3 << 2);
    frequentItemsubsets << (ItemIDList() << 1 << 3);
    result = RuleMiner::generateCandidateItemsets(frequentItemsubsets);
    QCOMPARE(result, QList<ItemIDList>());
    // Now (1,2,3) is the result.
    frequentItemsubsets.clear();
    frequentItemsubsets << (ItemIDList() << 1 << 2);
    frequentItemsubsets << (ItemIDList() << 2 << 3);
    frequentItemsubsets << (ItemIDList() << 1 << 3);
    result = RuleMiner::generateCandidateItemsets(frequentItemsubsets);
    QCOMPARE(result, QList<ItemIDList>() << (ItemIDList() << 1 << 2 << 3));
    // Now both (1,2,3) and (1,2,4) are in the result.
    frequentItemsubsets.clear();
    frequentItemsubsets << (ItemIDList() << 1 << 2);
    frequentItemsubsets << (ItemIDList() << 2 << 3);
    frequentItemsubsets << (ItemIDList() << 1 << 3);
    frequentItemsubsets << (ItemIDList() << 1 << 4);
    frequentItemsubsets << (ItemIDList() << 2 << 4);
    result = RuleMiner::generateCandidateItemsets(frequentItemsubsets);
    QCOMPARE(result, QList<ItemIDList>() << (ItemIDList() << 1 << 2 << 3)
                                         << (ItemIDList() << 1 << 2 << 4));
    
}
