#include "Constraints.h"

// TODO: Make the distinction between "standard usage" of Constraints (with
// preprocessing, so that we can match ItemIDLists very fast) versus the
// "alternative usage" of Constraints (without preprocessing, but it can work
// directly with strings; with QSet<ItemName>s).

namespace Analytics {

    const char * Constraints::ItemConstraintTypeName[2] = {
        "CONSTRAINT_POSITIVE",
        "CONSTRAINT_NEGATIVE",
    };



    //--------------------------------------------------------------------------
    // Public methods.

    Constraints::Constraints() {
        this->highestPreprocessedItemID = ROOT_ITEMID;
    }

    /**
     * Add an item constraint of a given constraint type. When
     * frequent itemsets are being generated, only those will be considered
     * that match the constraints defined here.
     * Wildcards are allowed, e.g. "episode:*" will match "episode:foo",
     * "episode:bar", etc.
     *
     * @param items
     *   An set of item names.
     * @param type
     *   The constraint type.
     */
    void Constraints::addItemConstraint(const QSet<ItemName> & items, ItemConstraintType type) {
        if (items.isEmpty())
            return;

        if (!this->itemConstraints.contains(type))
            this->itemConstraints.insert(type, QVector<QSet<ItemName> >());
        this->itemConstraints[type].append(items);
    }

    /**
     * Get all item IDs for a given item constraint type. Clearly, this only
     * returns item IDs after all item IDs have been preprocessed.
     *
     * @param type
     *   An item constraint type to get all item IDs for.
     * @return
     *   All item IDs for the given item constraint type.
     */
    QSet<ItemID> Constraints::getAllItemIDsForConstraintType(ItemConstraintType type) const {
        QSet<ItemID> preprocessedItemIDs;

        foreach (const QSet<ItemID> & itemIDs, this->preprocessedItemConstraints[type])
            preprocessedItemIDs.unite(itemIDs);

        return preprocessedItemIDs;
    }

    /**
     * Consider the given item ID -> name hash for use with constraints, but
     * only if its size exceeds the highest preprocessed item, or no items have
     * been preprocessed yet.
     *
     * This method is supposed to be used when all item IDs are already known.
     *
     * @param hash
     *   An item ID -> name hash.
     */
    void Constraints::preprocessItemIDNameHash(const ItemIDNameHash & hash) {
        if (this->highestPreprocessedItemID == ROOT_ITEMID || this->highestPreprocessedItemID + 1 < (uint) hash.size()) {
            this->clearPreprocessedItems();

            foreach (ItemID itemID, hash.keys()) {
                ItemName itemName = hash[itemID];
                this->preprocessItem(itemName, itemID);
            }
        }
    }

    /**
     * Consider the given item for use with constraints: store its item id in
     * an optimized data structure to allow for fast constraint checking
     * during frequent itemset generation.
     *
     * @param name
     *   An item name.
     * @param id
     *   The corresponding item ID.
     */
    void Constraints::preprocessItem(const ItemName & name, ItemID id) {
        QRegExp rx;
        rx.setPatternSyntax(QRegExp::Wildcard);

        // Store the item IDs that correspond to the wildcard item
        // constraints.
        ItemConstraintType constraintType;
        for (int i = CONSTRAINT_POSITIVE; i <= CONSTRAINT_NEGATIVE; i++) {
            constraintType = (ItemConstraintType) i;

            if (!this->itemConstraints.contains(constraintType))
                continue;

            for (int i = 0; i < this->itemConstraints[constraintType].size(); i++) {
                foreach (ItemName item, this->itemConstraints[constraintType][i]) {
                    // Map ItemNames to ItemIDs.
                    if (item.compare(name) == 0) {
                        this->addPreprocessedItemConstraint(constraintType, i, id);
                    }
                    // Map ItemNames with wildcards in them to *all* corresponding
                    // ItemIDs.
                    else if (item.contains('*')) {
                        rx.setPattern(item);
                        if (rx.exactMatch(name))
                            this->addPreprocessedItemConstraint(constraintType, i, id);
                    }
                }
            }
        }

        // Always keep the highest preprocessed item ID.
        if (this->highestPreprocessedItemID == ROOT_ITEMID || id > this->highestPreprocessedItemID)
            this->highestPreprocessedItemID = id;
    }

    /**
     * Remove the given item id from the optimized constraint storage data
     * structure, because it is infrequent.
     *
     * @param id
     *   The item id to remove.
     */
    void Constraints::removeItem(ItemID id) {
        ItemConstraintType type;
        for (int i = CONSTRAINT_POSITIVE; i <= CONSTRAINT_NEGATIVE; i++) {
            type = (ItemConstraintType) i;

            if (!this->preprocessedItemConstraints.contains(type))
                continue;

            for (int i = 0; i < this->itemConstraints[type].size(); i++)
                this->preprocessedItemConstraints[type][i].remove(id);
        }
    }

    /**
     * Check if the given itemset matches the defined constraints.
     *
     * @param itemset
     *   An itemset to check the constraints for.
     * @return
     *   True if the itemset matches the constraints, false otherwise.
     */
    bool Constraints::matchItemset(const ItemIDList & itemset) const {
#ifdef CONSTRAINTS_DEBUG
        FrequentItemset fis(itemset, 0);
        fis.IDNameHash = this->itemIDNameHash;
        qDebug() << "Matching itemset" << fis << " to constraints " << this->itemConstraints;
#endif
        bool match;
        for (int i = CONSTRAINT_POSITIVE; i <= CONSTRAINT_NEGATIVE; i++) {
            ItemConstraintType type = (ItemConstraintType) i;
            for (int c = 0; c < this->preprocessedItemConstraints[type].size(); c++) {
                match = Constraints::matchItemsetHelper(itemset, type, this->preprocessedItemConstraints[type][c]);
#ifdef CONSTRAINTS_DEBUG
                qDebug() << "\t" << Constraints::ItemConstraintTypeName[type] << c << ": " << match;
#endif
                if (!match)
                    return false;
            }
        }

        return true;
    }

    /**
     * Check if the given itemset matches the defined constraints.
     *
     * @param itemset
     *   An itemset to check the constraints for.
     * @return
     *   True if the itemset matches the constraints, false otherwise.
     */
    bool Constraints::matchItemset(const QSet<ItemName> & itemset) const {
#ifdef CONSTRAINTS_DEBUG
        qDebug() << "Matching itemset" << itemset << " to constraints " << this->itemConstraints;
#endif
        bool match;
        for (int i = CONSTRAINT_POSITIVE; i <= CONSTRAINT_NEGATIVE; i++) {
            ItemConstraintType type = (ItemConstraintType) i;
            for (int c = 0; c < this->itemConstraints[type].size(); c++) {
                match = Constraints::matchItemsetHelper(itemset, type, this->itemConstraints[type][c]);
#ifdef CONSTRAINTS_DEBUG
                qDebug() << "\t" << Constraints::ItemConstraintTypeName[type] << c << ": " << match;
#endif
                if (!match)
                    return false;
            }
        }

        return true;
    }

    /**
     * Check if a particular frequent itemset search space will be able to
     * match the defined constraints. We can do this by matching all
     * constraints over the itemset *and* prefix paths support counts
     * simultaneously (since this itemset will be extended with portions of
     * the prefix paths).
     *
     * @param itemset
     *   An itemset to check the constraints for.
     * @param prefixPathsSupportCounts
     *   A list of support counts for the prefix paths in this search space.
     * @return
     *   True if the itemset matches the constraints, false otherwise.
     */
    bool Constraints::matchSearchSpace(const ItemIDList & frequentItemset, const QHash<ItemID, SupportCount> & prefixPathsSupportCounts) const {
        for (int i = CONSTRAINT_POSITIVE; i <= CONSTRAINT_NEGATIVE; i++) {
            ItemConstraintType type = (ItemConstraintType) i;
            for (int c = 0; c < this->preprocessedItemConstraints[type].size(); c++) {
                if (!Constraints::matchSearchSpaceHelper(frequentItemset, prefixPathsSupportCounts, type, this->preprocessedItemConstraints[type][c]))
                    return false;
            }
        }

        return true;
    }


    //------------------------------------------------------------------------
    // Protected methods.

    /**
     * Helper function for Constraints::matchItemSet(ItemIDList).
     */
    bool Constraints::matchItemsetHelper(const ItemIDList & itemset, ItemConstraintType type, const QSet<ItemID> & constraintItems) {
        foreach (ItemID id, constraintItems) {
            switch (type) {

            case CONSTRAINT_POSITIVE:
                if (itemset.contains(id))
                    return true;
                break;

            case CONSTRAINT_NEGATIVE:
                if (itemset.contains(id))
                    return false;
                break;
            }
        }

        // In case we haven't returned yet, meaning that none of the items in
        // the constraint was actually *in* this itemset.
        switch (type) {
        case CONSTRAINT_NEGATIVE:
            return true;

        case CONSTRAINT_POSITIVE:
            return false;
        }

        // Satisfy the compiler.
        return false;
    }  

    /**
     * Helper function for Constraints::matchItemSet(QSet<ItemName>).
     */
    bool Constraints::matchItemsetHelper(const QSet<ItemName> & itemset, ItemConstraintType type, const QSet<ItemName> & constraintItems) {
        QSet<ItemName> itemsetCopy = itemset;
        bool emptyIntersection = itemsetCopy.intersect(constraintItems).isEmpty();

        if (type == CONSTRAINT_POSITIVE)
            return !emptyIntersection;
        else if (type == CONSTRAINT_NEGATIVE)
            return emptyIntersection;

        // Satisfy the compiler.
        return false;
    }

    /**
     * Helper function for Constraints::matchSearchSpace().
     */
    bool Constraints::matchSearchSpaceHelper(const ItemIDList & frequentItemset, const QHash<ItemID, SupportCount> & prefixPathsSupportCounts, ItemConstraintType type, const QSet<ItemID> & constraintItems) {
        foreach (ItemID id, constraintItems) {
            switch (type) {
            case CONSTRAINT_POSITIVE:
                if (frequentItemset.contains(id) || prefixPathsSupportCounts[id] > 0)
                    return true;
                break;

            case CONSTRAINT_NEGATIVE:
                if (prefixPathsSupportCounts[id] > 0)
                    return false;
                break;
            }
        }

        // In case we haven't returned yet, meaning that none of the items in
        // the constraint was actually *in* this itemset.
        switch (type) {
        case CONSTRAINT_NEGATIVE:
            return true;
            break;

        case CONSTRAINT_POSITIVE:
            return false;
            break;
        }

        // Satisfy the compiler.
        return false;
    }

    /**
     * Store a preprocessed item constraint in the optimized constraint data
     * structure.
     *
     * @param type
     *   The item constraint type.
     * @param constraint
     *   The how manieth constraint of this item constraint type.
     * @param id
     *   The item id.
     */
    void Constraints::addPreprocessedItemConstraint(ItemConstraintType type, uint constraint, ItemID id) {
        if (!this->preprocessedItemConstraints.contains(type))
            this->preprocessedItemConstraints.insert(type, QVector<QSet<ItemID> >());
        if ((uint) this->preprocessedItemConstraints[type].size() <= constraint)
            this->preprocessedItemConstraints[type].resize(constraint + 1);
        this->preprocessedItemConstraints[type][constraint].insert(id);
    }


    //------------------------------------------------------------------------
    // Other.

#ifdef DEBUG

    QDebug operator<<(QDebug dbg, const Constraints & constraints) {
        // Item constraints.
        // Stats.
        unsigned int sum = 0;
        foreach (ItemConstraintType type, constraints.itemConstraints.keys())
            for (int c = 0; c < constraints.itemConstraints[type].size(); c++)
                sum += constraints.itemConstraints[type][c].size();
        // Display.
        dbg.nospace() << "item constraints (" << sum << "):" << endl;
        for (int i = CONSTRAINT_POSITIVE; i <= CONSTRAINT_NEGATIVE; i++) {
            ItemConstraintType constraintType = (ItemConstraintType) i;
            dbg.nospace() << "\t" << Constraints::ItemConstraintTypeName[i] << ":" << endl;

            for (int c = 0; c < constraints.itemConstraints[constraintType].size(); c++) {
                dbg.space() << "\t\t" <<  c << ". ";
                if (constraints.itemConstraints[constraintType][c].isEmpty())
                    dbg.space() << "none";
                else
                    dbg.space() << constraints.itemConstraints[constraintType][c];
                dbg.nospace() << endl;
            }
            dbg.nospace() << endl;
        }


        // Preprocessed item constraints.
        // Stats.
        sum = 0;
        foreach (ItemConstraintType type, constraints.itemConstraints.keys())
            for (int c = 0; c < constraints.preprocessedItemConstraints[type].size(); c++)
                sum += constraints.preprocessedItemConstraints[type][c].size();
        // Display.
        dbg.nospace() << "preprocesseditem constraints (" << sum << "):" << endl;
        for (int i = CONSTRAINT_POSITIVE; i <= CONSTRAINT_NEGATIVE; i++) {
            ItemConstraintType constraintType = (ItemConstraintType) i;
            dbg.nospace() << "\t" << Constraints::ItemConstraintTypeName[i] << ":" << endl;

            for (int c = 0; c < constraints.preprocessedItemConstraints[constraintType].size(); c++) {
                dbg.space() << "\t\t" << c << ". ";
                if (constraints.preprocessedItemConstraints[constraintType][c].isEmpty())
                    dbg.space() << "none";
                else
                    dbg.space() << constraints.preprocessedItemConstraints[constraintType][c];
                dbg.nospace() << endl;
            }
            dbg.nospace() << endl;
        }

        return dbg.maybeSpace();
    }
#endif

}
