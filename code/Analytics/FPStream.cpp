#include "FPStream.h"

namespace Analytics {

    //----------------------------------------------------------------------
    // Public methods.

    FPStream::FPStream(double minSupport, double maxSupportError, ItemIDNameHash * itemIDNameHash, ItemNameIDHash * itemNameIDHash, ItemIDList * sortedFrequentItemIDs) {
        this->minSupport            = minSupport;
        this->maxSupportError       = maxSupportError;
        this->itemIDNameHash        = itemIDNameHash;
        this->itemNameIDHash        = itemNameIDHash;
        this->f_list                = sortedFrequentItemIDs;
        this->initialBatchProcessed = false;

        this->statusMutex.lock();
        this->processingBatch = false;
        this->currentBatchID  = 0;
        this->statusMutex.unlock();
    }

    bool FPStream::serialize(QTextStream & output) const {
        // Convert itemNameIDHash to a variant.
        QVariantMap itemNameIDHashVariant;
        foreach (ItemName itemName, this->itemNameIDHash->keys())
            itemNameIDHashVariant.insert(itemName, (int) this->itemNameIDHash->value(itemName));

        // Convert f_list to a variant.
        QList<QVariant> f_listVariant;
        foreach (const ItemID & itemID, *this->f_list)
            f_listVariant << QVariant(this->itemIDNameHash->value(itemID));

        QVariantMap json;
        json.insert("v", 1);
        json.insert("start time of first batch", (int) 0); // TODO
        json.insert("end time of last batch", (int) 0); // TODO
        json.insert("current batch ID", (int) this->currentBatchID); // Bug in QxtJSON prevents uints from being stringified.
        json.insert("transactions per batch", this->transactionsPerBatch.toVariantMap());
        json.insert("events per batch", this->eventsPerBatch.toVariantMap());
        json.insert("item name -> ID mapping", itemNameIDHashVariant);
        json.insert("frequent items by descending frequency", f_listVariant);

        output << QxtJSON::stringify(json) << "\n";

        this->patternTree.serialize(output, *this->itemIDNameHash);

        // TODO: add JSON conversion error checks.

        return true;
    }

    bool FPStream::deserialize(QTextStream & input) {
        // All data specific to FPStream is on the first line.
        QVariantMap json = QxtJSON::parse(input.readLine()).toMap();

        int version = json["v"].toInt();
        if (version == 1) {
            // TODO: support loading of state without restarting
            if (this->initialBatchProcessed || !this->itemIDNameHash->isEmpty() || this->patternTree.getNodeCount() > 0)
                qCritical("Data structures not empty. Restart the app to load.");

            bool success = true;

            // TODO: read "time of first batch" and "end time of last batch"
            this->initialBatchProcessed = true;
            this->currentBatchID = json["current batch ID"].toUInt();
            success = success && this->transactionsPerBatch.fromVariantMap(json["transactions per batch"].toMap());
            success = success && this->eventsPerBatch.fromVariantMap(json["events per batch"].toMap());
            if (!success)
                return false;

            QVariantMap itemNameIDHashVariant = json["item name -> ID mapping"].toMap();
            ItemID itemID;
            ItemName itemName;
            foreach (const QString & key, itemNameIDHashVariant.keys()) {
                itemName = (ItemName) key;
                itemID = (ItemID) itemNameIDHashVariant[key].toUInt();
                this->itemIDNameHash->insert(itemID, itemName);
                this->itemNameIDHash->insert(itemName, itemID);
            }

            // f_list; but since it contains strings only (EpisodeNames), this
            // also means we can (and *must*!) rebuild the item name -> id and
            // vice versa hashes.
            QList<QVariant> f_listValues = json["frequent items by descending frequency"].toList();
            foreach (const QVariant & variant, f_listValues) {
                itemName = (ItemName) variant.toString();
                itemID = this->itemNameIDHash->value(itemName);

                // Consider this item for use with constraints.
                this->constraints.preprocessItem(itemName, itemID);
                this->constraintsToPreprocess.preprocessItem(itemName, itemID);

                this->f_list->append(itemID);
            }

            // All remaining data is for the PatternTree data structure.
            this->patternTree.deserialize(input, this->itemIDNameHash, *this->itemNameIDHash, this->currentBatchID);
        }

        // TODO: add JSON conversion error checks.

        return true;
    }


    //----------------------------------------------------------------------
    // Public slots.

    /**
     * Calculate the minimum support for frequent itemsets for a range of
     * buckets, based on the batch sizes.
     *
     * @param from
     *   The range starts at this bucket.
     * @param to
     *   The range ends at this bucket.
     * @return
     *   The minimum support for this range.
     */
    SupportCount FPStream::calculateMinSupportForRange(uint from, uint to) const {
        return ceil(this->minSupport * this->eventsPerBatch.getSupportForRange(from, to));
    }

    /**
     * Process a batch of transactions. Each batch should cover a 15-minute
     * window (i.e. a quarter).
     *
     * @param transactions
     *   A batch of transactions.
     * @param transactionsPerEvent
     *   The number of transactions per event. Necessary to determine the
     *   correct minimum absolute support when a single event is expanded
     *   into multiple transactions.
     * @param startNewTimeWindow
     *   Whether to start a new time window (in each pattern's TiltedTimeWindow
     *   object) or not. This will only true when entering a new time window,
     *   i.e. not when the data for a time window is split in multiple batches,
     *   to reduce memory consumption and improve performance.
     * @param lastChunkOfBatch
     *   Whether this is the last chunk of the batch of transactions for this
     *   time window. (i.e. the next chunk will be for the batch of the next
     *   time window.)
     */
    void FPStream::processBatchTransactions(const QList<QStringList> & transactions, double transactionsPerEvent, bool startNewTimeWindow, bool lastChunkOfBatch) {
        this->statusMutex.lock();
        this->processingBatch = true;
        this->lastChunkOfBatch = lastChunkOfBatch;
        if (startNewTimeWindow)
            this->currentBatchID++;
        this->statusMutex.unlock();

        // Store the batch sizes. By storing it in a tilted time window, they
        // will automatically be summed in the same way as any other tilted
        // time window's support counts.
        this->transactionsPerBatch.appendQuarter(transactions.size(), this->currentBatchID);
        this->eventsPerBatch.appendQuarter(transactions.size() / transactionsPerEvent, this->currentBatchID);

        // Mine the frequent itemsets in this batch.
        this->currentFPGrowth = new FPGrowth(transactions, (SupportCount) (this->maxSupportError * transactions.size() / transactionsPerEvent), this->itemIDNameHash, this->itemNameIDHash, this->f_list);
//        this->currentFPGrowth = new FPGrowth(transactions, (SupportCount) ceil(this->minSupport * transactions.size() / transactionsPerEvent), this->itemIDNameHash, this->itemNameIDHash, this->f_list);
        this->currentFPGrowth->setConstraints(this->constraints);
        this->currentFPGrowth->setConstraintsForRuleConsequents(this->constraintsToPreprocess);

        // Initial batch.
        if (!this->initialBatchProcessed) {
            // Calculate frequent itemsets synchronously using FPGrowth.
            QList<FrequentItemset> frequentItemsets = this->currentFPGrowth->mineFrequentItemsets(FPGROWTH_SYNC);
            delete this->currentFPGrowth;

            // Add all frequent itemsets to the PatternTree.
            foreach (FrequentItemset frequentItemset, frequentItemsets)
                this->patternTree.addPattern(frequentItemset, this->currentBatchID);

            this->initialBatchProcessed = true;

            this->statusMutex.lock();
            this->processingBatch = false;
            this->statusMutex.unlock();

            emit batchProcessed();
        }
        // Subsequent batches.
        else {
            // Subsequent batches are processed on a per-frequent itemset
            // basis and therefor they use the signal/slot mechanism (to
            // decide on a per-frequent itemset basis if supersets should
            // be mined as well). Hence, subsequent batches are handled by
            // FPStream::processFrequentItemset().

            // Keep track of the current quarter we're in, in case we're
            // starting a new TiltedTimeWindow (by adding a new pattern to the
            // PatternTree).
            if (startNewTimeWindow)
                this->patternTree.nextQuarter();

            // Connect signals/slots using queued connections, because TODO.
            // TODO: make Qt::QueuedConnection actually work, by running
            // FPGrowth in another thread.
            connect(this->currentFPGrowth, SIGNAL(minedFrequentItemset(FrequentItemset,bool,const FPTree*)), this, SLOT(processFrequentItemset(FrequentItemset,bool,const FPTree*))/*, Qt::QueuedConnection*/);
            connect(this->currentFPGrowth, SIGNAL(branchCompleted(ItemIDList)), this, SLOT(branchCompleted(ItemIDList))/*, Qt::QueuedConnection*/);
            connect(this, SIGNAL(mineForFrequentItemsupersets(const FPTree*,FrequentItemset)), this->currentFPGrowth, SLOT(generateFrequentItemsets(const FPTree*,FrequentItemset))/*, Qt::QueuedConnection*/);

#ifdef FPSTREAM_DEBUG
            qDebug() << "Subsequent batch: " << this->currentBatchID;
#endif

            // The initial suffix is empty.
            this->statusMutex.lock();
            ItemIDList empty;
            this->supersetsBeingCalculated.append(empty);
            this->statusMutex.unlock();

            // Calculate frequent itemsets asynchronously using FPGrowth.
            this->currentFPGrowth->mineFrequentItemsets(FPGROWTH_ASYNC);
        }
    }

    /**
     * Process a single frequent itemset: update the pattern tree with the
     * information it carries (possibly adding it to the pattern tree).
     *
     * Note: this function performs all substeps described in step 3.(a) of
     *       the FP-Stream algorithm.
     *
     * @param frequentItemset
     *   A frequent itemset, also known as a "pattern".
     * @param frequentItemsetMatchesConstraints
     *   Whether the frequent itemset ("pattern") matches the constraints or
     *   not. In some cases, the superset of this frequent itemset still has
     *   the potential to match the constraints. Then it is possible thanks to
     *   this parameter to *not* update the pattern tree with the given
     *   frequentItemset, but to still decide to continue mining its supersets
     *   because that still may lead to useful results.
     * @param ctree
     *   Pointer to a conditional FP-Tree. If this pointer equals NULL, then
     *   there either is no conditional FP-Tree, or it simply does not have
     *   the potential to result in supersets that may match the constraints.
     *   Either way, when ctree equals NULL, there is nothing left to explore.
     */
    void FPStream::processFrequentItemset(const FrequentItemset & frequentItemset, bool frequentItemsetMatchesConstraints, const FPTree * ctree) {
#ifdef FPSTREAM_DEBUG
        qDebug() << "\t\t\t\tProcessing frequent itemset" << frequentItemset << ", matches constraints: " << frequentItemsetMatchesConstraints;
#endif

        TiltedTimeWindow * tiltedTimeWindow;
        Granularity dropTailStartGranularity;

        // Get the tilted time window for the current pattern.
        tiltedTimeWindow = this->patternTree.getPatternSupport(frequentItemset.itemset);

        // If the current pattern exists in the pattern tree.
        if (tiltedTimeWindow != NULL) {
            // Add the frequent itemset to the pattern tree.
            this->patternTree.addPattern(frequentItemset, this->currentBatchID);

            // *Pretend* to conduct tail pruning.
            // In this modified version of FP-Stream, we actually only really
            // perform tail pruning when we're in the last chunk of the batch;
            // in preceding chunks of the batch, we only *calculate* the part of
            // the tail that can be dropped. We use this information instead of
            // checking whether the TiltedTimeWindow is empty to decide whether
            // the supersets should be calculated. This is not exactly the same
            // as the original algorithm, but it's a close approximation that
            // allows for parallellization and/or processing chunks of batches,
            // thus allowing for more efficiency and/or less memory consumption.
            dropTailStartGranularity = FPStream::calculateDroppableTail(*tiltedTimeWindow, this->minSupport, this->maxSupportError, this->eventsPerBatch);
            if (dropTailStartGranularity != (Granularity) -1) {
                this->statusMutex.lock();
                bool lastChunkOfBatch = this->lastChunkOfBatch;
                this->statusMutex.unlock();
                if (lastChunkOfBatch)
                    tiltedTimeWindow->dropTail(dropTailStartGranularity);
            }

            // If the tilted time window is empty, then tell FP-Growth to
            // stop mining supersets of this frequent itemset (type II
            // pruning) by simply not asking it to continue to mine for
            // supersets.
            // Conversely, when the tilted time window is *not* empty, let
            // FP-Growth know it should continue to mine supersets. But if
            // the received ctree (conditional tree) is NULL, then it was
            // determined through constraint search space matching that it
            // would be impossible to find frequent supersets that match the
            // constraints.
            if (dropTailStartGranularity != (Granularity) 0 && ctree != NULL) {
                this->statusMutex.lock();
                this->supersetsBeingCalculated.append(frequentItemset.itemset);
                this->statusMutex.unlock();
#ifdef FPSTREAM_DEBUG
                qDebug() << "\t\t\t\t\tbranch started (a):" << frequentItemset.itemset << ", remainder: " << this->supersetsBeingCalculated << "(" << this->supersetsBeingCalculated.size() << ")";
#endif
                // TODO: potential optimization: make sure that the !isEmpty()
                // check on tiltedTimeWindow is run before the constraint
                // search space matching is performed in
                // FPGrowth::considerFrequentItemsupersets().
                // Because clearly, the latter is far more expensive than
                // the former. This will require some further refactoring,
                // though.
                emit this->mineForFrequentItemsupersets(ctree, frequentItemset);
            }
            else {
                // Delete the conditional FP-Tree since it will not be used
                // anyway.
                if (ctree != NULL)
                    delete ctree;
#ifdef FPSTREAM_DEBUG
                qDebug() << "\t\t\t\ttype II pruning applied!";
#endif
            }
        }
        // If the current pattern does not yet exist in the pattern
        // tree.
        else if (tiltedTimeWindow == NULL) {
            // Delete the conditional FP-Tree since it will not be used
            // anyway.
            if (ctree != NULL)
                delete ctree;

            // Perform the regular processing (as described by the FP-Stream
            // algorithm) only when the frequent itemset matched the
            // contraints *OR* when its superset has the potential to match
            // the constraints.
            // The latter is necessary because *if* the superset matches the
            // constraints, then its antecedent's SupportCount also needs to
            // to be known, to be able to calculate the confidence of
            // potential association rules.
            if (frequentItemsetMatchesConstraints || ctree != NULL) {
                // Add it (it meets the minimum support minus the error rate
                // because it was returned by FP-Growth).
                this->patternTree.addPattern(frequentItemset, this->currentBatchID);
            }

            // Note: this also applies type I pruning: this pattern was not
            // yet found in the pattern tree, and thus none of its supersets
            // need be examined (due to the fact that no
            // mineForFrequentItemsupersets() signal is being emitted, none
            // of its supersets will get examined).
#ifdef FPSTREAM_DEBUG
            qDebug() << "\t\t\t\ttype I pruning applied!";
#endif
        }
    }

    /**
     * Every time a branch (or subtree) of the FPTree has been mined for
     * frequent itemsets by FPGrowth, a signal is emitted that is received by
     * this slot. This allows us to determine termination of the asynchronous
     * execution of the FPGrowth algorithm.
     * When termination is detected, @fn{FPStream::updateUnaffectedNodes()} is
     * called.
     *
     * @param itemset
     *   The itemset whose branch was completed (i.e. for which all supersets
     *   have been evaluated).
     */
    void FPStream::branchCompleted(const ItemIDList & itemset) {
        QMutexLocker(&this->statusMutex);

        // removeOne suffises, since each frequent itemset is only added
        // once.
        this->supersetsBeingCalculated.removeOne(itemset);
#ifdef FPSTREAM_DEBUG
        qDebug() << "\t\t\t\t\tbranch completed:" << itemset << ", remainder: " << this->supersetsBeingCalculated << "(" << this->supersetsBeingCalculated.size() << ")";
#endif

        // If there are no more frequent itemsets for which supersets are
        // being calculated, then that means we've finished mining frequent
        // itemsets with FP-Growth.
        if (this->supersetsBeingCalculated.isEmpty()) {
#ifdef FPSTREAM_DEBUG
            qDebug() << "CLOOOOOOOOOOOOOOOOOOOOOSING UP!";
#endif

            this->statusMutex.lock();
            bool lastChunkOfBatch = this->lastChunkOfBatch;
            this->statusMutex.unlock();
            if (lastChunkOfBatch) {
                // Since all frequent itemsets have been mined and processed, we
                // should now update nodes in the pattern tree that remained
                // unaffected during this batch.
                this->updateUnaffectedNodes(this->patternTree.getRoot());
            }

            // Now the processing of this batch is officially over.
            this->processingBatch = false;

            // We don't need to manually disconnect all signals and slots:
            // that will happen automatically as soon as the fpgrowth object
            // is deleted.
            delete this->currentFPGrowth;

#ifdef FPSTREAM_DEBUG
            qDebug() << "\tPatternTree size: " << this->patternTree.getNodeCount();
            qDebug() << "\tItemIDNameHash size: " << this->itemIDNameHash->size();
            qDebug() << "\tf_list size: " << this->f_list->size();
#endif

            emit batchProcessed();
        }
    }


    //----------------------------------------------------------------------
    // Protected static methods.

    /**
     * Calculate how much of the tail can be dropped.
     *
     * @return
     *   -1 if nothing is droppable, a position in the [0, TTW_NUM_BUCKETS-1]
     *   range if a tail can be dropped.
     */
    Granularity FPStream::calculateDroppableTail(const TiltedTimeWindow & window, double minSupport, double maxSupportError, const TiltedTimeWindow & batchSizes) {
        Q_ASSERT(window.oldestBucketFilled <= batchSizes.oldestBucketFilled);

        // If it's empty, there can't be anything worth calculating.
        if (window.oldestBucketFilled == -1)
            return (Granularity) -1;

        // Iterate over all buckets in the tilted time window, starting at the
        // tail (i.e. the last/oldest bucket).
        int n = window.oldestBucketFilled;
        int l = -1, m = -1;
        SupportCount cumulativeSupport = 0, cumulativeBatchSize = 0;
        for (int i = n; i >= 0; i--) {
            // Ignore unused buckets: continue.
            if (window.buckets[i] == (unsigned int) TTW_BUCKET_UNUSED)
                continue;

            // Continue going to the front of the vector as long as the
            // support of each bucket does not meet the minimum support, while
            // storing the frontmost bucket index that does not meet the
            // minimum support in the variable "l".
            if (window.buckets[i] < ceil(minSupport * batchSizes.buckets[i]))
                l = i;
            else
                break;
        }

        // The variable "l" now contains the youngest bucket that does not
        // meet the minimum support.

        // If no such l is found, then there is no tail that can be dropped.
        if (l == -1)
            return (Granularity) -1;

        // Iterate again over all buckets in the tilted time window, starting
        // at the tail (i.e. the last/oldest bucket).
        for (int i = n; i >= l; i--) {
            // Ignore unused buckets: continue.
            if (window.buckets[i] == (unsigned int) TTW_BUCKET_UNUSED)
                continue;

            cumulativeBatchSize += batchSizes.buckets[i];
            cumulativeSupport   += window.buckets[i];

            // Continue going to the front of the vector as long as the
            // cumulative support does not  meet the corresponding cumulative
            // maximum support error, while storing the frontmost bucket index
            // that does not meet the cumulative maximum support error in the
            // variable "m".
            if (cumulativeSupport < ceil(maxSupportError * cumulativeBatchSize))
                m = i;
            else
                break;
        }

        // The variable "m" now contains the youngest bucket that does not
        // meet the minimum support *nor* the cumulative maximum support error
        // unless no bucket could be found that does not meet the cumulative
        // maximum support error, in that case "m" will still have its default
        // value of -1, which is then a correct answer too: there is *no*
        // droppable tail in that case.

        // If m > -1, that means there are buckets that can be dropped.
        // However, to ensure that TiltedTimeWindows stay in sync, we can only
        // drop entire granularities. Hence, find the lowest granularity after
        // m that can be dropped in its entirety (when m corresponds to the
        // first bucket of a granularity, it is of course *that* granularity
        // that can be dropped).
        Granularity g;
        if (m > -1) {
            for (g = (Granularity) 0; g < (Granularity) TTW_NUM_GRANULARITIES; g = (Granularity) ((int) g + 1))
                if ((uint) m <= TiltedTimeWindow::GranularityBucketOffset[g] && (uint) m < TiltedTimeWindow::GranularityBucketOffset[(Granularity) g + 1])
                    return g;
        }

        return (Granularity) -1;
    }


    //----------------------------------------------------------------------
    // Protected methods.

    /**
     * Update the nodes that have remained unaffected during the processing
     * of the current batch.
     *
     * Note: this function performs all substeps described in step 3.(b) of
     *       the FP-Stream algorithm.
     *
     * @param node
     *   A node in the PatternTree. Initially the root node, but in recursive
     *   calls, each node will be checked and if necessary, updated.
     */
    void FPStream::updateUnaffectedNodes(FPNode<TiltedTimeWindow> * node) {
        // (b) Scan the pattern tree (depth-first), and, for each
        // encountered pattern:
        // 1) insert 0 into its tilted time window if it wasn't updated
        //    for the current batch of transactions (i.e. the current
        //    timeslot).
        // AND perform tail pruning on its tilted time window; when it is
        //    empty after tail pruning and it is a leaf, drop it
        // (i.e. only for itemsets where a 0 was inserted)

        if (node == NULL)
            return;

        const QHash<ItemID, FPNode<TiltedTimeWindow> *> children = node->getChildren();
        foreach (const FPNode<TiltedTimeWindow> * child, children.values()) {
            this->updateUnaffectedNodes(const_cast<FPNode<TiltedTimeWindow> *>(child));
        }

        // There's nothing to update in the root node.
        if (node->getItemID() == ROOT_ITEMID)
            return;

        // If this frequent itemset (and thus its tilted time window) was
        // not updated during the current batch, then update it now.
        TiltedTimeWindow * tiltedTimeWindow = node->getPointerToValue();
        if (tiltedTimeWindow->getLastUpdate() != this->currentBatchID) {
            tiltedTimeWindow->appendQuarter(0, this->currentBatchID);

            // Conduct tail pruning.
            Granularity dropTailStartGranularity = FPStream::calculateDroppableTail(*tiltedTimeWindow, this->minSupport, this->maxSupportError, this->eventsPerBatch);
            if (dropTailStartGranularity != (Granularity) -1)
                tiltedTimeWindow->dropTail(dropTailStartGranularity);

            // If this node is a leaf node and its tilted time window is
            // empty, then drop the leaf.
            if (node->isLeaf() && tiltedTimeWindow->isEmpty())
                this->patternTree.removePattern(node);
        }
    }
}
