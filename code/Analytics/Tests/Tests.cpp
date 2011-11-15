#include "TestFPTree.h"
#include "TestConstraints.h"
#include "TestFPGrowth.h"
#include "TestRuleMiner.h"
#include "TestTTWDefinition.h"
#include "TestTiltedTimeWindow.h"
#include "TestPatternTree.h"
#include "TestFPStream.h"

int main() {
    TestFPTree FPTree;
    QTest::qExec(&FPTree);

    TestConstraints constraints;
    QTest::qExec(&constraints);

    TestFPGrowth FPGrowth;
    QTest::qExec(&FPGrowth);

    TestRuleMiner ruleMiner;
    QTest::qExec(&ruleMiner);

    // FP-Stream related classes & tests.
    TestTTWDefinition ttwDefinition;
    QTest::qExec(&ttwDefinition);

    TestTiltedTimeWindow tiltedTimeWindow;
    QTest::qExec(&tiltedTimeWindow);

    TestPatternTree patternTree;
    QTest::qExec(&patternTree);

    TestFPStream FPStream;
    QTest::qExec(&FPStream);

    return 0;
}
