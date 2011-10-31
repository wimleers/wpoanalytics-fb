#include "Analyst.h"

namespace Analytics {

    Analyst::Analyst(double minSupport, double maxSupportError, double minConfidence) {
        this->setParameters(minSupport, maxSupportError, minConfidence);

        this->currentQuarterID = 0;

        // Stats for the UI.
        this->allBatchesNumPageViews = 0;
        this->allBatchesNumTransactions = 0;
        this->allBatchesStartTime = 0;

#ifdef DEBUG
        this->frequentItemsetItemConstraints.itemIDNameHash = &this->itemIDNameHash;
        this->ruleAntecedentItemConstraints.itemIDNameHash = &this->itemIDNameHash;
        this->ruleConsequentItemConstraints.itemIDNameHash = &this->itemIDNameHash;
#endif

        this->fpstream = new FPStream(this->minSupport, this->maxSupportError, &this->itemIDNameHash, &this->itemNameIDHash, &this->sortedFrequentItemIDs);
        connect(this->fpstream, SIGNAL(batchProcessed()), this, SLOT(fpstreamProcessedBatch()));
    }

    Analyst::~Analyst() {
        delete this->fpstream;
    }

    void Analyst::setParameters(double minSupport, double maxSupportError, double minConfidence) {
        this->minSupport      = minSupport;
        this->maxSupportError = maxSupportError;
        this->minConfidence   = minConfidence;
    }

    void Analyst::resetConstraints() {
        this->frequentItemsetItemConstraints.reset();
        this->ruleAntecedentItemConstraints.reset();
        this->ruleConsequentItemConstraints.reset();
    }

    /**
     * Add a frequent itemset item constraint of a given constraint type. When
     * frequent itemsets are being generated, only those will be considered
     * that match the constraints defined here.
     *
     * @param items
     *   An set of item names.
     * @param type
     *   The constraint type.
     */
    void Analyst::addFrequentItemsetItemConstraint(QSet<ItemName> items, ItemConstraintType type) {
        this->frequentItemsetItemConstraints.addItemConstraint(items, type);
    }

    /**
     * Add a rule antecedent item constraint of a given constraint type. When
     * rules are being mined, only those will be considered that match the
     * constraints defined here.
     *
     * @param items
     *   An set of item names.
     * @param type
     *   The constraint type.
     */
    void Analyst::addRuleAntecedentItemConstraint(QSet<ItemName> items, ItemConstraintType type) {
        // If an item is required to be in the rule antecedent, it evidently
        // must also be in the frequent itemsets. Therefore, the same item
        // constraints that apply to the rule consequents also apply to
        // frequent itemsets.
        // By also applying these item constraints to frequent itemset
        // generation, we reduce the amount of work to be done to a minimum.
        this->frequentItemsetItemConstraints.addItemConstraint(items, type);

        this->ruleAntecedentItemConstraints.addItemConstraint(items, type);
    }

    /**
     * Add a rule consequent item constraint of a given constraint type. When
     * rules are being mined, only those will be considered that match the
     * constraints defined here.
     *
     * @param items
     *   An set of item names.
     * @param type
     *   The constraint type.
     */
    void Analyst::addRuleConsequentItemConstraint(QSet<ItemName> items, ItemConstraintType type) {
        // If an item is required to be in the rule consequent, it evidently
        // must also be in the frequent itemsets. Therefore, the same item
        // constraints that apply to the rule consequents also apply to
        // frequent itemsets.
        // By also applying these item constraints to frequent itemset
        // generation, we reduce the amount of work to be done to a minimum.
        this->frequentItemsetItemConstraints.addItemConstraint(items, type);

        this->ruleConsequentItemConstraints.addItemConstraint(items, type);
    }

    /**
     * Override of QObject::moveToThread(), to automatically also move the
     * FPStream object to the other thread.
     */
    void Analyst::moveToThread(QThread * thread) {
        QObject::moveToThread(thread);
        this->fpstream->moveToThread(thread);
    }

    /**
     * Extract the episode from an itemset and convert all item IDs to item
     * names. Essential for the UI.
     *
     * @param itemset
     *   The itemset (of item IDs) from which to extract the episode.
     * @return
     *   A pair: the first parameter is the episode item, the second parameter
     *   is a list that contains the remaining antecedent attributes.
     */
    QPair<ItemName, ItemNameList> Analyst::extractEpisodeFromItemset(ItemIDList itemset) const {
        ItemNameList itemNames;
        ItemName episodeName;

        // First, map to item names.
        foreach (ItemID id, itemset)
            itemNames.append(this->itemIDNameHash[id]);

        // Next, filter out the episode item.
        foreach (ItemName name, itemNames) {
            if (name.startsWith("episode")) {
                episodeName = name;
                itemNames.removeAll(name);
                break;
            }
        }

        return qMakePair(episodeName, itemNames);
    }

    /**
     * Convert all item IDs in an itemset to item names. Essential for the UI.
     *
     * @param itemset
     *   The itemset (of item IDs) from which to extract the episode.
     * @return
     *   A list of consequent attributes.
     */
    ItemNameList Analyst::itemsetIDsToNames(ItemIDList itemset) const {
        ItemNameList itemNames;

        foreach (ItemID id, itemset)
            itemNames.append(this->itemIDNameHash[id]);

        return itemNames;
    }


    //------------------------------------------------------------------------
    // Public slots.

    void Analyst::analyzeTransactions(const QList<QStringList> &transactions, double transactionsPerEvent, Time start, Time end, quint32 quarterID, bool lastChunkOfBatch) {
        // Stats for the UI.
        this->currentBatchStartTime = start;
        this->currentBatchEndTime = end;
        this->currentBatchNumPageViews = ((transactionsPerEvent == 0) ? 0 : (transactions.size() / transactionsPerEvent));
        this->currentBatchNumTransactions = transactions.size();
        this->timer.start();

        // Necessary to be able to update the browsable concept hierarchy in
        // Analyst::fpstreamProcessedBatch().
        this->uniqueItemsBeforeMining = this->itemIDNameHash.size();

        // Notify the UI.
        emit analyzing(true, this->currentBatchStartTime, this->currentBatchEndTime, this->currentBatchNumPageViews, this->currentBatchNumTransactions);

        // Perform the actual mining.
        this->performMining(transactions, transactionsPerEvent, quarterID, lastChunkOfBatch);

        // Since the mining above is performed asynchronously, this is NOT the
        // place where we know the calculations end. Only FP-Stream can know,
        // hence FP-Stream's processedBatch() signal is the correct indicator.
        // This signal is then sent from the @function fpstreamProcessedBatch()
        // slot.
    }

    /**
     * Mine rules over a range of buckets (i.e., a range of time).
     *
     * @param from
     *   The range starts at this bucket.
     * @param to
     *   The range ends at this bucket.
     */
    void Analyst::mineRules(uint from, uint to) {
        // Notify the UI.
        emit mining(true);

        this->timer.start();

        // First, consider each item for use with constraints.
        this->frequentItemsetItemConstraints.preprocessItemIDNameHash(this->itemIDNameHash);
        this->ruleAntecedentItemConstraints.preprocessItemIDNameHash(this->itemIDNameHash);
        this->ruleConsequentItemConstraints.preprocessItemIDNameHash(this->itemIDNameHash);

        // Now, mine for association rules.
        QList<AssociationRule> associationRules = RuleMiner::mineAssociationRules(
                this->fpstream->getPatternTree().getFrequentItemsetsForRange(
                        this->fpstream->calculateMinSupportForRange(from, to),
                        this->frequentItemsetItemConstraints,
                        from,
                        to
                ),
                this->minConfidence,
                this->ruleAntecedentItemConstraints,
                this->ruleConsequentItemConstraints,
                this->fpstream->getPatternTree(),
                from,
                to
        );

        int duration = this->timer.elapsed();

        // Notify the UI.
        emit mining(false);
        emit ruleMiningStats(
                    duration,
                    from,
                    to,
                    associationRules.size(),
                    this->fpstream->getNumTransactionsInRange(from, to),
                    this->fpstream->getNumEventsInRange(from, to)
        );
        emit minedRules(from, to, associationRules, this->fpstream->getNumEventsInRange(from, to));
    }

    void Analyst::mineAndCompareRules(uint fromOlder, uint toOlder, uint fromNewer, uint toNewer) {
        // Notify the UI.
        emit mining(true);

        this->timer.start();

        // First, consider each item for use with constraints.
        this->frequentItemsetItemConstraints.preprocessItemIDNameHash(this->itemIDNameHash);
        this->ruleConsequentItemConstraints.preprocessItemIDNameHash(this->itemIDNameHash);

        // Now, mine the association rules for the "older" range.
        QList<AssociationRule> olderRules = RuleMiner::mineAssociationRules(
                this->fpstream->getPatternTree().getFrequentItemsetsForRange(
                        this->fpstream->calculateMinSupportForRange(fromOlder, toOlder),
                        this->frequentItemsetItemConstraints,
                        fromOlder,
                        toOlder
                ),
                this->minConfidence,
                this->ruleConsequentItemConstraints,
                this->ruleConsequentItemConstraints,
                this->fpstream->getPatternTree(),
                fromOlder,
                toOlder
        );

        // Now, mine the association rules for the "newer" range.
        QList<AssociationRule> newerRules = RuleMiner::mineAssociationRules(
                this->fpstream->getPatternTree().getFrequentItemsetsForRange(
                        this->fpstream->calculateMinSupportForRange(fromNewer, toNewer),
                        this->frequentItemsetItemConstraints,
                        fromNewer,
                        toNewer
                ),
                this->minConfidence,
                this->ruleConsequentItemConstraints,
                this->ruleConsequentItemConstraints,
                this->fpstream->getPatternTree(),
                fromNewer,
                toNewer
        );

        // Finally, compare the rules for the "older" and "newer" range.
        TiltedTimeWindow const * const eventsPerBatch = this->fpstream->getEventsPerBatch();
        SupportCount supportForNewerRange = eventsPerBatch->getSupportForRange(fromNewer, toNewer);
        SupportCount supportForOlderRange = eventsPerBatch->getSupportForRange(fromOlder, toOlder);
        // Calculate the number of events in the intersected range. The two time
        // ranges may either overlap (i.e. have an intersection) or not (i.e. be
        // disjoint).
        // e.g.:
        //   * 2-4, 5-8 or 5-8, 2-4 -> disjoint
        //   * 2-5, 4-8 or 4-8, 2-5 -> intersection
        bool olderTimeRangeIsActuallyOlder = (fromOlder <= fromNewer);
        uint actualOlderFrom = (olderTimeRangeIsActuallyOlder) ? fromOlder : fromNewer;
        uint actualOlderTo   = (olderTimeRangeIsActuallyOlder) ? toOlder : toNewer;
        uint actualNewerFrom = (olderTimeRangeIsActuallyOlder) ? fromNewer : fromOlder;
        uint actualNewerTo   = (olderTimeRangeIsActuallyOlder) ? toNewer : toOlder;
        SupportCount supportForIntersectedRange;
        if (actualOlderTo > actualNewerFrom) // Overlap
            supportForIntersectedRange = eventsPerBatch->getSupportForRange(actualOlderFrom, actualNewerTo);
        else
            supportForIntersectedRange = supportForOlderRange + supportForNewerRange;

        QList<AssociationRule> comparedRules;
        QList<Confidence> confidenceVariance;
        QList<float> supportVariance;

        // Intersected rules.
        QList<AssociationRule> intersectedRules = newerRules.toSet().intersect(olderRules.toSet()).toList();
        comparedRules.append(intersectedRules);
        int n, o;
        foreach (const AssociationRule & rule, intersectedRules) {
            n = newerRules.indexOf(rule);
            o = olderRules.indexOf(rule);
            confidenceVariance.append(newerRules[n].confidence - olderRules[o].confidence);
            supportVariance.append((1.0 * newerRules[n].support / supportForNewerRange) - (1.0 * olderRules[o].support / supportForOlderRange));
        }

        // Newer-only rules.
        QList<AssociationRule> newerOnlyRules = newerRules.toSet().subtract(intersectedRules.toSet()).toList();
        comparedRules.append(newerOnlyRules);
        for (int i = 0; i < newerOnlyRules.size(); i++) {
            confidenceVariance.append(1.0);
            supportVariance.append(1.0);
        }

        // Older-only rules.
        QList<AssociationRule> olderOnlyRules = olderRules.toSet().subtract(intersectedRules.toSet()).toList();
        comparedRules.append(olderOnlyRules);
        for (int i = 0; i < olderOnlyRules.size(); i++) {
            confidenceVariance.append(-1.0);
            supportVariance.append(-1.0);
        }

        int duration = this->timer.elapsed();

        // Notify the UI.
        emit mining(false);
        emit ruleMiningStats(
                    duration,
                    fromOlder,
                    toOlder,
                    olderRules.size() + newerRules.size() + comparedRules.size(),
                    this->fpstream->getNumTransactionsInRange(fromOlder, toOlder) + this->fpstream->getNumTransactionsInRange(fromNewer, toNewer),
                    this->fpstream->getNumEventsInRange(fromOlder, toOlder) + this->fpstream->getNumEventsInRange(fromNewer, toNewer)
        );
        emit comparedMinedRules(fromOlder, toOlder,
                                fromNewer, toNewer,
                                intersectedRules,
                                olderRules,
                                newerRules,
                                comparedRules,
                                confidenceVariance,
                                supportVariance,
                                supportForIntersectedRange,
                                supportForNewerRange,
                                supportForOlderRange);
    }

    /**
     * Load the Analyst state from a file.
     *
     * @param fileName
     *   The name of the file to load from, or ":stdin" to load from stdin.
     */
    void Analyst::load(QString fileName) {
        QFile file;
        bool opened = false;

        if (fileName == ":stdin")
            opened = file.open(stdin, QIODevice::ReadOnly | QIODevice::Text);
        else {
            file.setFileName(fileName);
            opened = file.open(QIODevice::ReadOnly | QIODevice::Text);
        }

        if (!opened) {
            qCritical("Could not open file %s for reading.", qPrintable(fileName));
            emit loaded(false, 0, 0, 0, 0, 0, 0, 0);
        }
        else {
            QTextStream input(&file);
            bool success = this->fpstream->deserialize(input);
            file.close();
            emit loaded(
                success,
                0,
                0,
                this->fpstream->getEventsPerBatch()->getSupportForRange(0, TTW_NUM_BUCKETS-1),
                this->fpstream->getTransactionsPerBatch()->getSupportForRange(0, TTW_NUM_BUCKETS-1),
                this->fpstream->getItemIDNameHash()->size(),
                this->fpstream->getF_list()->size(),
                this->fpstream->getPatternTreeSize()
            );
            emit newItemsEncountered(this->itemIDNameHash);
        }
    }

    /**
     * Save the Analyst state to a file.
     *
     * @param fileName
     *   The name of the file to save to, or ":stdout" to save to stdout.
     */
    void Analyst::save(QString fileName) {
        QFile file;
        bool opened = false;

        if (fileName == ":stdout")
            opened = file.open(stdout, QIODevice::WriteOnly | QIODevice::Text);
        else {
            file.setFileName(fileName);
            opened = file.open(QIODevice::WriteOnly | QIODevice::Text);
        }

        if (!opened) {
            qCritical("Could not open file %s for writing.", qPrintable(fileName));
            emit saved(false);
        }
        else {
            QTextStream out(&file);
            bool success = this->fpstream->serialize(out);
            file.close();
            emit saved(success);
        }
    }


    //------------------------------------------------------------------------
    // Protected slots.

    void Analyst::fpstreamProcessedBatch() {
        int duration = this->timer.elapsed();
        if (this->allBatchesStartTime == 0)
            this->allBatchesStartTime = this->currentBatchStartTime;
        this->allBatchesNumPageViews += this->currentBatchNumPageViews;
        this->allBatchesNumTransactions += this->currentBatchNumTransactions;

        // Update the browsable concept hierarchy.
        emit newItemsEncountered(this->itemIDNameHash);

        emit stats(
                    duration,
                    this->allBatchesStartTime,
                    this->currentBatchEndTime,
                    this->currentBatchNumPageViews,
                    this->currentBatchNumTransactions,
                    this->itemIDNameHash.size(),
                    this->fpstream->getNumFrequentItems(),
                    this->fpstream->getPatternTreeSize()
        );
        emit analyzing(false, 0, 0, 0, 0);
        emit processedBatch();
    }


    //------------------------------------------------------------------------
    // Protected methods.

    void Analyst::performMining(const QList<QStringList> & transactions, double transactionsPerEvent, quint32 quarterID, bool lastChunkOfBatch) {
        bool fpstream = true;
        bool startNewTimeWindow;

        if (!fpstream) {
//            qDebug() << "----------------------> FPGROWTH";
        // Clear these every time, to ensure the original behavior.
        this->itemIDNameHash.clear();
        this->itemNameIDHash.clear();
        this->sortedFrequentItemIDs.clear();

        qDebug() << "starting mining, # transactions: " << transactions.size();
        FPGrowth * fpgrowth = new FPGrowth(transactions, ceil(this->minSupport * transactions.size() / transactionsPerEvent), &this->itemIDNameHash, &this->itemNameIDHash, &this->sortedFrequentItemIDs);
        fpgrowth->setConstraints(this->frequentItemsetItemConstraints);
        fpgrowth->setConstraintsForRuleConsequents(this->ruleConsequentItemConstraints);
        QList<FrequentItemset> frequentItemsets = fpgrowth->mineFrequentItemsets(false);
        qDebug() << "frequent itemset mining complete, # frequent itemsets:" << frequentItemsets.size();

        /*
        this->ruleConsequentItemConstraints = fpgrowth->getPreprocessedConstraints();
        QList<AssociationRule> associationRules = RuleMiner::mineAssociationRules(frequentItemsets, this->minConfidence, this->ruleConsequentItemConstraints, fpgrowth);
        qDebug() << "mining association rules complete, # association rules:" << associationRules.size();

        qDebug() << associationRules;
        */

        delete fpgrowth;

        } else {
//            qDebug() << "----------------------> FPSTREAM";

        // Temporary show the FPStream datastructure (PatternTree) as it's
        // being built, until rule mining has also been implemented.
        static bool initial = true;
        if (initial) {
            this->fpstream->setConstraints(this->frequentItemsetItemConstraints);
            this->fpstream->setConstraintsToPreprocess(this->ruleConsequentItemConstraints);
            initial = false;
        }
        if (quarterID != this->currentQuarterID) {
            this->currentQuarterID = quarterID;
            startNewTimeWindow = true;
        }
        else
            startNewTimeWindow = false;
        this->fpstream->processBatchTransactions(transactions, transactionsPerEvent, startNewTimeWindow, lastChunkOfBatch);
        /*
        qDebug() << this->fpstream->getPatternTree().getNodeCount();
        qDebug() << this->itemIDNameHash.size() << this->itemNameIDHash.size() << this->sortedFrequentItemIDs.size();
        qDebug() << this->fpstream->getPatternTree();
        */
        }
    }
}
