#ifndef ITEM_H
#define ITEM_H

#include <QHash>
#include <QList>
#include <QString>
#include <QMetaType>

#ifdef DEBUG
#include <QDebug>
#endif


namespace Analytics {

    /**
     * Generic data mining types.
     */
    // Supports 2^32 *different* items. Upgradable to quint64.
    typedef quint32 ItemID;
    // Largest supported value for quint32.
    #define ROOT_ITEMID 4294967295
    typedef QString ItemName;
    // Supports 2^32 count. Upgradable to quint64.
    typedef quint32 SupportCount;
    #define MAX_SUPPORT 4294967295
    typedef float Confidence;
    typedef QHash<ItemID, ItemName> ItemIDNameHash;
    typedef QHash<ItemName, ItemID> ItemNameIDHash;
    struct Item {
        Item() {}
        Item(ItemID id)
            : id(id), supportCount(1) {}
        Item(ItemID id, SupportCount supportCount)
            : id(id), supportCount(supportCount) {}

        ItemID id;
        /**
         * One would not expect SupportCount to be associated with an item.
         * However, this allows for cleaner code when building conditional
         * FP-trees. More specifically: the prefix paths that
         * FPTree::calculatePrefixPaths() returns already include the correct
         * SupportCount values (i.e. the number of times that itemset was
         * included in all transactions) and can be passed to
         * FPTree::addTransaction() *directly*. Otherwise, we'd have to
         * repeatedly insert the prefix path, to match the number of times
         * that itemset was included in all transactions.
         * Each item occurs once in each transaction. Therefor, this defaults
         * to 1.
         */
        SupportCount supportCount;

#ifdef DEBUG
        Item(ItemID id, const ItemIDNameHash * IDNameHash)
            : id(id), supportCount(1), IDNameHash(IDNameHash) {}
        Item(ItemID id, SupportCount supportCount, const ItemIDNameHash * IDNameHash)
            : id(id), supportCount(supportCount), IDNameHash(IDNameHash) {}
        const ItemIDNameHash * IDNameHash;
#endif
    };
    inline bool operator==(const Item & i1, const Item & i2) {
        // Important! We don't require a match on the supportCount attribute!
        return i1.id == i2.id;
    }
    inline bool operator!=(const Item & i1, const Item & i2) {
        return !(i1 == i2);
    }


    /**
     * Generic data mining container types.
     */
    typedef QList<ItemID> ItemIDList;
    typedef QList<Item> ItemList;
    typedef QList<ItemName> ItemNameList;
    typedef QList<Item> Transaction;
    struct FrequentItemset {
        FrequentItemset() : support(0) {}
        FrequentItemset(ItemIDList itemset, SupportCount support)
            : itemset(itemset), support(support) {}
        FrequentItemset(ItemList itemset) {
            SupportCount minSupport = MAX_SUPPORT;
            foreach (Item item, itemset) {
                this->itemset.append(item.id);
                minSupport = (item.supportCount < minSupport) ? item.supportCount : minSupport;
            }
            this->support = minSupport;
        }
        // This constructor can be used while generating new candidate
        // frequent itemsets.
        FrequentItemset(ItemID itemID, SupportCount itemIDSupport, const FrequentItemset & suffix) {
            this->itemset.append(itemID);
            this->itemset.append(suffix.itemset);
            this->support = (itemIDSupport < suffix.support || suffix.itemset.isEmpty()) ? itemIDSupport : suffix.support;
        }

        ItemIDList itemset;
        SupportCount support;

#ifdef DEBUG
        FrequentItemset(ItemIDList itemset, SupportCount support, ItemIDNameHash * IDNameHash)
            : itemset(itemset), support(support), IDNameHash(IDNameHash) {}

        const ItemIDNameHash * IDNameHash;
#endif
    };
    inline bool operator==(const FrequentItemset & fis1, const FrequentItemset & fis2) {
        // Important! We don't require a match on the supportCount attribute!
        return fis1.support == fis2.support && fis1.itemset == fis2.itemset;
    }
    inline bool operator!=(const FrequentItemset & fis1, const FrequentItemset & fis2) {
        return !(fis1 == fis2);
    }
    struct AssociationRule {
        AssociationRule() {}
        AssociationRule(ItemIDList antecedent, ItemIDList consequent, SupportCount support, Confidence confidence)
            : antecedent(antecedent), consequent(consequent), support(support), confidence(confidence) {}

        ItemIDList antecedent;
        ItemIDList consequent;
        SupportCount support;
        Confidence confidence;

#ifdef DEBUG
        const ItemIDNameHash * IDNameHash;
#endif
    };
    uint qHash(const AssociationRule & r);
    inline bool operator==(const AssociationRule & r1, const AssociationRule & r2) {
        // Important! We don't require a match on the support and consequent attributes!
        return r1.antecedent == r2.antecedent && r1.consequent == r2.consequent;
    }
    inline bool operator!=(const AssociationRule & r1, const AssociationRule & r2) {
        return !(r1 == r2);
    }

    void registerBasicMetaTypes();

#ifdef DEBUG
    // QDebug() streaming output operators.
    QDebug operator<<(QDebug dbg, const Item & item);
    QDebug operator<<(QDebug dbg, const ItemIDList & pattern);
    QDebug operator<<(QDebug dbg, const Transaction & transaction);
    QDebug operator<<(QDebug dbg, const FrequentItemset & frequentItemset);
    QDebug operator<<(QDebug dbg, const AssociationRule & associationRule);
    QDebug itemIDHelper(QDebug dbg, const ItemIDList & itemset, ItemIDNameHash const * const IDNameHash);
#endif

}

Q_DECLARE_METATYPE(Analytics::ItemIDList)
Q_DECLARE_METATYPE(Analytics::FrequentItemset)
Q_DECLARE_METATYPE(Analytics::ItemIDNameHash)


#endif // ITEM_H
