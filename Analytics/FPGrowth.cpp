#include "FPGrowth.h"

namespace Analytics {

    FPGrowth::FPGrowth(const QList<QStringList> & transactions, SupportCount minSupportAbsolute, ItemIDNameHash * itemIDNameHash, ItemNameIDHash * itemNameIDHash, ItemIDList * sortedFrequentItemIDs) {
        this->itemIDNameHash        = itemIDNameHash;
        this->itemNameIDHash        = itemNameIDHash;
        this->sortedFrequentItemIDs = sortedFrequentItemIDs;

        this->transactions = transactions;

        this->minSupportAbsolute = minSupportAbsolute;

        this->tree = new FPTree();
#ifdef DEBUG
        this->tree->itemIDNameHash = this->itemIDNameHash;
#endif
    }

    FPGrowth::~FPGrowth() {
        delete this->tree;
    }

    /**
     * Mine frequent itemsets. (First scan the transactions, then build the
     * FP-tree, then generate the frequent itemsets from there.)
     *
     * @param asynchronous
     *   See the explanation for the identically named parameter of @fn
     *   generateFrequentItemsets().
     * @return
     *   The frequent itemsets that were found.
     */
    QList<FrequentItemset> FPGrowth::mineFrequentItemsets(bool asynchronous) {
        this->scanTransactions();
        this->buildFPTree();
        return this->generateFrequentItemsets(this->tree, FrequentItemset(), asynchronous);
    }

    /**
     * Calculate the support count for an itemset.
     *
     * @param itemset
     *   The itemset to calculate the support count for.
     * @return
     *   The support count for this itemset
     */
    SupportCount FPGrowth::calculateSupportCount(const ItemIDList & itemset) const {
        // For itemsets of size 1, we can simply use the QHash that contains
        // all frequent items' support counts, since it contains the exact
        // data we need (this is FPGrowth::totalFrequentSupportCounts).
        // For larger itemsets, we'll have to get the exact support count by
        // examining the FP-tree.
        if (itemset.size() == 1) {
            return this->totalFrequentSupportCounts[itemset[0]];
        }
        else {
            // First optimize the itemset so that the item with the least
            // support is the last item.
            ItemIDList optimizedItemset = this->orderItemsetBySupport(itemset);

            // Starting with the last item in the itemset:
            // 1) calculate its prefix paths
            // 2) filter these prefix paths to remove the items along these
            //    paths that don't meet the minimum support
            // 3) build the corresponding conditional FP-tree
            // Repeat this until we've reached the second item in the itemset,
            // then we have the support count for this item set.
            int last = optimizedItemset.size() - 1;
            FPTree * cfptree = NULL;
            QList<ItemList> prefixPaths;
            for (int whichItem = last; whichItem > 0; whichItem--) {
                // Step 1: calculate prefix paths.
                if (whichItem == last)
                    prefixPaths = this->tree->calculatePrefixPaths(optimizedItemset[whichItem]);
                else {
                    prefixPaths = cfptree->calculatePrefixPaths(optimizedItemset[whichItem]);
                    delete cfptree;
                }
                // Step 2: filter.
                prefixPaths = FPGrowth::filterPrefixPaths(prefixPaths, this->minSupportAbsolute);
                // Step 3: build the conditional FP-tree.
                // Note that it is impossible to end with zero prefix paths
                // after filtering, since the itemset that is passed to this
                // function consists of frequent items.
                cfptree = new FPTree();
#ifdef DEBUG
                cfptree->itemIDNameHash = this->itemIDNameHash;
#endif
                cfptree->buildTreeFromPrefixPaths(prefixPaths);
            }

            // The conditional FP-tree for the second item in the itemset
            // contains the support count for the itemset that was passed into
            // this function.
            return cfptree->getItemSupport(itemset[0]);
        }
    }


    //------------------------------------------------------------------------------
    // Protected slots.

    /**
     * Slot that receives a Transaction, optimizes it and adds it to the tree.
     *
     * @param transaction
     *   The transaction to process.
     */
    void FPGrowth::processTransaction(const Transaction & transaction) {
        Transaction optimizedTransaction;
        optimizedTransaction = this->optimizeTransaction(transaction);

        // It's possible that the optimized transaction has become empty if
        // none of the items in the given transaction meet or exceed the
        // minimum support.
        if (optimizedTransaction.size() > 0)
            this->tree->addTransaction(optimizedTransaction);
    }


    //------------------------------------------------------------------------
    // Protected static methods.

    /**
     * Given an ItemID -> SupportCount hash, sort ItemIDs by decreasing
     * support count.
     *
     * @param itemSupportCounts
     *   A QHash of ItemID -> SupportCount pairs.
     * @param ignoreList
     *   A list of ItemIDs to ignore (i.e. not include in the result).
     * @return
     *   A QList of ItemIDs, sorted by decreasing support count.
     */
    ItemIDList FPGrowth::sortItemIDsByDecreasingSupportCount(const QHash<ItemID, SupportCount> & itemSupportCounts, const ItemIDList * const ignoreList){
        QHash<SupportCount, ItemID> itemIDsBySupportCount;
        QSet<SupportCount> supportCounts;
        SupportCount supportCount;

        foreach (ItemID itemID, itemSupportCounts.keys()) {
            if (ignoreList->contains(itemID))
                continue;

            supportCount = itemSupportCounts[itemID];

            // Fill itemsBySupport by using QHash::insertMulti(), which allows
            // for multiple values for the same key.
            itemIDsBySupportCount.insertMulti(supportCount, itemID);

            // Fill supportCounts. Since this is a set, each unique support
            // count will only be stored once.
            supportCounts.insert(supportCount);
        }

        // Sort supportCounts from smaller to greater. But first convert from
        // a QSet to a QList, because sets cannot have an order by definition.
        QList<SupportCount> sortedSupportCounts;
        sortedSupportCounts = supportCounts.toList();
//        qSort(sortedSupportCounts);
        qSort(sortedSupportCounts.begin(), sortedSupportCounts.end(), qGreater<SupportCount>());

        // Store all ItemIDs, sorted by support count. If multiple ItemIDs
        // have the same SupportCount, sort them from small to large.
        ItemIDList sortedItemIDs;
        foreach (SupportCount support, sortedSupportCounts) {
            ItemIDList itemIDs = itemIDsBySupportCount.values(support);
            qSort(itemIDs);
            sortedItemIDs.append(itemIDs);
        }

        return sortedItemIDs;
    }

    /**
     * Remove items from the prefix paths based on the support counts of the
     * items *within* the prefix paths.
     *
     * @param prefixPaths
     *   The prefix paths to filter.
     * @param minSupportAbsolute
     *   The minimum absolute support count that should be met.
     * @return
     *   The filtered prefix paths.
     */
    QList<ItemList> FPGrowth::filterPrefixPaths(const QList<ItemList> & prefixPaths, SupportCount minSupportAbsolute) {
        QHash<ItemID, SupportCount> prefixPathsSupportCounts = FPTree::calculateSupportCountsForPrefixPaths(prefixPaths);

        QList<ItemList> filteredPrefixPaths;
        ItemList filteredPrefixPath;
        foreach (ItemList prefixPath, prefixPaths) {
            foreach (Item item, prefixPath) {
                if (prefixPathsSupportCounts[item.id] >= minSupportAbsolute)
                    filteredPrefixPath.append(item);
            }
            if (filteredPrefixPath.size() > 0)
                filteredPrefixPaths.append(filteredPrefixPath);
            filteredPrefixPath.clear();
        }

        return filteredPrefixPaths;
    }


    //------------------------------------------------------------------------
    // Protected methods.

    /**
     * Preprocess the transactions:
     * 1) map the item names to item IDs, so we only have to store numeric IDs
     *    in the FP-tree and conditional FP-trees, instead of entire strings
     * 2) determine the support count of each item (happens simultaneously
     *    with step 1)
     * 3) discard infrequent items' support count
     * 4) sort the frequent items by decreasing support count
     *
     * Also, each time when a new item name is mapped to an item id, it is
     * processed for use in constraints as well.
     */
    void FPGrowth::scanTransactions() {
        // Consider items with item names that have been mapped to item IDs
        // in previous executions of FPGrowth for use with constraints.
        if (!this->itemIDNameHash->isEmpty()) {
            foreach (ItemID itemID, this->itemIDNameHash->keys()) {
                ItemName itemName = this->itemIDNameHash->value(itemID);
                this->constraints.preprocessItem(itemName, itemID);
                this->constraintsForRuleConsequents.preprocessItem(itemName, itemID);
            }
        }

        // Map the item names to item IDs. Maintain two dictionaries: one for
        // each look-up direction (name -> id and id -> name).
        ItemID itemID;
        foreach (QStringList transaction, this->transactions) {
            foreach (QString itemName, transaction) {
                // Look up the itemID for this itemName, or create it.
                if (!this->itemNameIDHash->contains(itemName)) {
                    itemID = this->itemNameIDHash->size();
                    this->itemNameIDHash->insert(itemName, itemID);
                    this->itemIDNameHash->insert(itemID, itemName);
                    this->totalFrequentSupportCounts.insert(itemID, 0);

                    // Consider this item for use with constraints.
                    this->constraints.preprocessItem(itemName, itemID);
                    this->constraintsForRuleConsequents.preprocessItem(itemName, itemID);
                }
                else
                    itemID = this->itemNameIDHash->value(itemName);

                this->totalFrequentSupportCounts[itemID]++;
            }
        }

        // Discard infrequent items' SupportCount.
        foreach (itemID, this->totalFrequentSupportCounts.keys()) {
            if (this->totalFrequentSupportCounts[itemID] < this->minSupportAbsolute) {
                this->totalFrequentSupportCounts.remove(itemID);

                // Remove infrequent items' ids from the preprocessed
                // constraints.
                this->constraints.removeItem(itemID);
                this->constraintsForRuleConsequents.removeItem(itemID);
            }
        }

        // Sort the frequent items' item ids by decreasing support count.
        this->sortedFrequentItemIDs->append(FPGrowth::sortItemIDsByDecreasingSupportCount(this->totalFrequentSupportCounts, this->sortedFrequentItemIDs));

#ifdef FPGROWTH_DEBUG
        qDebug() << "order:";
        foreach (itemID, *(this->sortedFrequentItemIDs)) {
            if (this->totalFrequentSupportCounts.contains(itemID))
                qDebug() << this->totalFrequentSupportCounts[itemID] << " times: " << Item(itemID, this->itemIDNameHash);
        }
#endif
    }

    /**
     * Build the FP-tree, by using the results from scanTransactions().
     *
     * TODO: figure out if this can be done in one pass with scanTransactions.
     * This should be doable since we can use the generated itemIDs as they
     * are being created.
     */
    void FPGrowth::buildFPTree() {
        Transaction transaction;

        foreach (QStringList transactionAsStrings, this->transactions) {
            transaction.clear();
            foreach (ItemName name, transactionAsStrings) {
#ifdef DEBUG
                transaction << Item((ItemID) this->itemNameIDHash->value(name), this->itemIDNameHash);
#else
                transaction << Item((ItemID) this->itemNameIDHash->value(name));
#endif
            }

            // The transaction in QStringList form has been converted to
            // QList<Item> form. Now process the transaction in this form.
            this->processTransaction(transaction);
        }

#ifdef FPGROWTH_DEBUG
        qDebug() << "Parsed" << this->transactions.size() << "transactions.";
        qDebug() << *this->tree;
#endif
    }

    /**
     * Optimize a transaction.
     *
     * This is achieved by sorting the items by decreasing support count. To
     * do this as fast as possible, this->sortedFrequentItemIDs is reused.
     * Because this->sortedFrequentItemIDs does not include infrequent items,
     * these are also automatically removed by this simple routine.
     * However, that's not all: we want to ensure that item IDs for positive
     * rule consequent constraints are at the top of the FP-tree. Because the
     * mining process employed by FP-Growth ensures that we start at the leaf
     * nodes and gradually grow frequent itemsets to include nodes towards the
     * the root of the tree, that means that all subsets of frequent itemsets
     * that do match the constraints are generated first. This implies that
     * supersets of frequent itemsets match the constraints. We can then
     * exploit this property to ensure all frequent itemsets that correspond
     * to rule antecedents are generated before the frequent itemsets that
     * correspond to entire rules. This is necessary if we want to use
     * FP-Stream for rule mining: if we don't do this, we won't know what the
     * support is for rule antecedents.
     *
     * @param transaction
     *   A transaction.
     * @return
     *   The optimized transaction.
     */
    Transaction FPGrowth::optimizeTransaction(const Transaction & transaction) const {
        Transaction optimizedTransaction, front, back;
        Item item;
        QSet<ItemID> frontItemIDs;

        // Determine which items should be at the front.
        frontItemIDs.unite(this->constraintsForRuleConsequents.getAllItemIDsForConstraintType(ItemConstraintPositive));

        foreach (ItemID itemID, *(this->sortedFrequentItemIDs)) {
            item.id = itemID;
            if (transaction.contains(item)) {
                // Ensure items are either sent to the front or the back.
                if (frontItemIDs.contains(itemID))
                    front.append(transaction.at(transaction.indexOf(item)));
                else
                    back.append(transaction.at(transaction.indexOf(item)));
            }
        }

        // Build the optimized transaction by gluing together front and back.
        optimizedTransaction << front << back;

        return optimizedTransaction;
    }

    /**
     * Optimize an itemset (ItemIDList), like @fn{FPGrowth::optimizedTransaction}.
     *
     * We need this in @fn{FPGrowth::calculateSupportCountExactly}, to ensure
     * that we calculate the support by gradually moving from the leaf nodes
     * towards the root nodes. We can only achieve that by ensuring we
     * traverse the tree in an identical manner.
     *
     * @param itemset
     *   An itemset.
     * @return
     *   The optimized itemset.
     */
    ItemIDList FPGrowth::optimizeItemset(const ItemIDList & itemset) const {
        ItemIDList optimizedItemset, front, back;
        QSet<ItemID> frontItemIDs;

        // Determine which items should be at the front.
        frontItemIDs.unite(this->constraintsForRuleConsequents.getAllItemIDsForConstraintType(ItemConstraintPositive));

        foreach (ItemID itemID, *(this->sortedFrequentItemIDs)) {
            if (itemset.contains(itemID)) {
                // Ensure items are either sent to the front or the back.
                if (frontItemIDs.contains(itemID))
                    front.append(itemID);
                else
                    back.append(itemID);
            }
        }

        // Build the optimized itemset by gluing together front and back.
        optimizedItemset << front << back;

        return optimizedItemset;
    }

    /**
     * Optimize an ItemIDList by ordering its items by descending support.
     *
     * @param itemset
     *   A list of ItemIDs.
     * @return
     *   The optimized list of ItemIDs.
     */
    ItemIDList FPGrowth::orderItemsetBySupport(const ItemIDList & itemset) const {
        ItemIDList optimizedItemset;

        foreach (ItemID itemID, *(this->sortedFrequentItemIDs)) {
            if (itemset.contains(itemID))
                optimizedItemset.append(itemID);
        }

        return optimizedItemset;
    }

    /**
     * Generate the frequent itemsets recursively
     *
     * @param ctree
     *   Initially the entire FP-tree, but in subsequent (recursive) calls,
     *   a conditional FP-tree.
     * @param suffix
     *   The current frequent itemset suffix. Empty in the initial call, but
     *   automatically filled by this function when it recurses.
     * @param asynchronous
     *   FPGROWTH_ASYNC or FPGROWTH_SYNC
     *   By default, this method runs in a non-blocking (asynchronous)
     *   manner: it emits a signal for every frequent itemset it finds at a
     *   level. It is then up to the recipient of this signal to call this
     *   method again (i.e. to cause recursion) if it wants supersets of the
     *   signaled frequent itemset to be mined.
     *   When passing FPGROWTH_SYNC to this parameter, this method will run
     *   in a blocking (synchronous) manner, and will thus not allow another
     *   object to decide if supersets should also be mined. It will collect
     *   all frequent itemsets that can be found and then return all of them
     *   at once.
     * @return
     *   The entire list of frequent itemsets. The SupportCount for each Item
     *   still needs to be cleaned: it should be set to the minimum of all
     *   Items in each frequent itemset.
     */
    QList<FrequentItemset> FPGrowth::generateFrequentItemsets(const FPTree * ctree, const FrequentItemset & suffix, bool asynchronous) {
        bool frequentItemsetMatchesConstraints;
        QList<FrequentItemset> frequentItemsets;
        ItemIDList itemIDsInTree = ctree->getItemIDs();
        static QSet<ConstraintClassification> constraintsSubset;

        if (constraintsSubset.isEmpty()) {
            constraintsSubset.insert(ConstraintClassificationMonotone);
        }

        // Now iterate over each of the ordered suffix items and generate
        // candidate frequent itemsets!
        foreach (ItemID prefixItemID, itemIDsInTree) {
            // Only if this prefix item's support meets or exceeds the minimum
            // support, it will be added as a frequent itemset (appended with
            // the received suffix of course).
            SupportCount prefixItemSupport = ctree->getItemSupport(prefixItemID);
            if (prefixItemSupport >= this->minSupportAbsolute) {
                // The current suffix item, when prepended to the received
                // suffix, is the next frequent itemset.
                // Additionally, this new frequent itemset will become the
                // next recursion's suffix.
                FrequentItemset frequentItemset(prefixItemID, prefixItemSupport, suffix);
#ifdef DEBUG
                frequentItemset.IDNameHash = this->itemIDNameHash;
#endif

                // Check if there are supersets to be mined.
                FPTree * cfptree = this->considerFrequentItemsupersets(ctree, frequentItemset.itemset);

                // Only store the current frequent itemset if it matches the
                // constraints *OR* when its superset has the potential to match
                // the constraints.
                frequentItemsetMatchesConstraints = this->constraints.matchItemset(frequentItemset.itemset, constraintsSubset);
                if (!asynchronous && (frequentItemsetMatchesConstraints || cfptree != NULL)) {
                    frequentItemsets.append(frequentItemset);
#ifdef FPGROWTH_DEBUG
                qDebug() << "\t\t\t\t new frequent itemset:" << frequentItemset;
#endif
                }

                // If there are supersets to be mined, mine them.
                if (cfptree != NULL && !asynchronous) {
                    // Attempt to generate more frequent itemsets, with the
                    // current frequent itemset as the suffix.
                    frequentItemsets.append(this->generateFrequentItemsets(cfptree, frequentItemset, asynchronous));

                    // This will make sure every conditional FP-tree gets
                    // deleted, but *not* the original tree. This is exactly
                    // what we want, since the original tree will be deleted
                    // in the destructor.
                    // This is the synchronous case, i.e. the case where no
                    // signals are emitted.
                    delete cfptree;
                }

                if (asynchronous)
                    emit this->minedFrequentItemset(frequentItemset, frequentItemsetMatchesConstraints, cfptree);
            }
        }

        if (asynchronous) {
            // This will make sure every conditional FP-tree gets deleted,
            // but *not* the original tree. This is exactly what we want,
            // since the original tree will be deleted in the destructor.
            // This is the asynchronous case, i.e. the case where no signals
            // are emitted.
            if (ctree != this->tree)
                delete ctree;

            // Necessary to terminate the algorithm in the asynchronous case.
            emit this->branchCompleted(suffix.itemset);
        }

        return frequentItemsets;
    }

    FPTree * FPGrowth::considerFrequentItemsupersets(const FPTree * ctree, const ItemIDList & frequentItemset) {
        // Calculate the prefix paths for the current prefix item
        // (which is a prefix to the current suffix, but when
        // calculating prefix paths, it's actually considered the
        // leading item ID of the suffix, i.e. as if it were the
        // leading item ID of the future suffix, which is in fact the
        // frequent itemset that we've just found).
        QList<ItemList> prefixPaths = ctree->calculatePrefixPaths(frequentItemset[0]);

        // Remove items from the prefix paths that no longer have sufficient
        // support.
        // (i.e. remove items *within* prefix paths; this keeps every prefix
        // path, unless of course it becomes empty, then it is discarded.)
        prefixPaths = FPGrowth::filterPrefixPaths(prefixPaths, this->minSupportAbsolute);

        // If no prefix paths remain after filtering, we won't be able
        // to generate any further frequent item sets.
        if (!prefixPaths.isEmpty()) {
            // If the conditional FP-tree would not be able to match the
            // constraints (which we can know by looking at the current
            // frequent itemset and the prefix paths support counts), then
            // just don't bother generating it.
            // This is effectively pruning the search space for frequent
            // itemsets.
            QHash<ItemID, SupportCount> prefixPathsSupportCounts = FPTree::calculateSupportCountsForPrefixPaths(prefixPaths);
            if (!this->constraints.matchSearchSpace(frequentItemset, prefixPathsSupportCounts))
                return NULL;

            // Build the conditional FP-tree for these prefix paths,
            // by creating a new FP-tree and pretending the prefix
            // paths are transactions.
            FPTree * cfptree = new FPTree();
#ifdef DEBUG
            cfptree->itemIDNameHash = this->itemIDNameHash;
#endif
            cfptree->buildTreeFromPrefixPaths(prefixPaths);
#ifdef FPGROWTH_DEBUG
            qDebug() << *cfptree;
#endif

            return cfptree;
        }
        else
            return NULL;
    }
}
