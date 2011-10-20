#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QObject>
#include <QTime>
#include <QList>
#include <QHash>
#include <QStringList>
#include <QPair>

#include <QThread>
#include <QWaitCondition>
#include <QMutex>

#include "Item.h"
#include "Constraints.h"
#include "FPGrowth.h"
#include "FPStream.h"
#include "RuleMiner.h"

typedef uint Time;

namespace Analytics {

    class Analyst : public QObject {
        Q_OBJECT

    public:
        Analyst(double minSupport, double maxSupportError, double minConfidence);
        ~Analyst();
        void addFrequentItemsetItemConstraint(ItemName item, ItemConstraintType type);
        void addRuleConsequentItemConstraint(ItemName item, ItemConstraintType type);

        // Override moveToThread to also move the FPStream instance.
        void moveToThread(QThread * thread);

        int getPatternTreeSize() const { return this->fpstream->getPatternTreeSize(); }

        // UI integration.
        QPair<ItemName, ItemNameList> extractEpisodeFromItemset(ItemIDList itemset) const;
        ItemNameList itemsetIDsToNames(ItemIDList itemset) const;

    signals:
        // Signals for UI.
        void analyzing(bool, Time start, Time end, quint64 pageViews, quint64 transactions);
        void analyzedDuration(int duration);
        void stats(Time start, Time end, quint64 pageViews, quint64 transactions, quint64 uniqueItems, quint64 frequentItems, quint64 patternTreeSize);
        void mining(bool);
        void minedDuration(int duration);
        void loaded(bool success);
        void saved(bool success);
        void newItemsEncountered(Analytics::ItemIDNameHash itemIDNameHash);

        // Signals for calculations.
        void processedBatch();
        void minedRules(uint from, uint to, QList<Analytics::AssociationRule> associationRules, Analytics::SupportCount eventsInTimeRange);
        void comparedMinedRules(uint fromOlder, uint toOlder,
                                uint fromNewer, uint toNewer,
                                QList<Analytics::AssociationRule> intersectedRules,
                                QList<Analytics::AssociationRule> olderRules,
                                QList<Analytics::AssociationRule> newerRules,
                                QList<Analytics::AssociationRule> comparedRules,
                                QList<Analytics::Confidence> confidenceVariance,
                                QList<float> supportVariance,
                                Analytics::SupportCount eventsInIntersectedTimeRange,
                                Analytics::SupportCount eventsInOlderTimeRange,
                                Analytics::SupportCount eventsInNewerTimeRange);

    public slots:
        void analyzeTransactions(const QList<QStringList> & transactions, double transactionsPerEvent, Time start, Time end, quint32 quarterID, bool lastChunkOfBatch);
        void mineRules(uint from, uint to);
        void mineAndCompareRules(uint fromOlder, uint toOlder, uint fromNewer, uint toNewer);
        void load(QString fileName);
        void save(QString fileName);

    protected slots:
        void fpstreamProcessedBatch();

    protected:
        void performMining(const QList<QStringList> & transactions, double transactionsPerEvent, quint32 quarterID, bool lastChunkOfBatch);

        FPStream * fpstream;
        double minSupport;
        double maxSupportError;
        double minConfidence;
        quint32 currentQuarterID;

        Constraints frequentItemsetItemConstraints;
        Constraints ruleConsequentItemConstraints;

        ItemIDNameHash itemIDNameHash;
        ItemNameIDHash itemNameIDHash;
        ItemIDList sortedFrequentItemIDs;

        // Stats for the UI.
        uint currentBatchStartTime;
        uint currentBatchEndTime;
        quint64 currentBatchNumPageViews;
        quint64 currentBatchNumTransactions;
        quint64 allBatchesStartTime;
        quint64 allBatchesNumPageViews;
        quint64 allBatchesNumTransactions;
        QTime timer;

        int uniqueItemsBeforeMining;
    };
}

#endif // ANALYST_H
