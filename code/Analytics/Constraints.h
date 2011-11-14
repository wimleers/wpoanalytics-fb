#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include <QRegExp>
#include <QList>
#include <QSet>
#include <QVector>

#include "Item.h"


namespace Analytics {

#ifdef DEBUG
//    #define CONSTRAINTS_DEBUG 0
#endif

    enum ItemConstraintType {
        ItemConstraintPositive,
        ItemConstraintNegative
    };

    // TODO: not yet implemented.
    enum LengthConstraintType {
        LengthConstraintEquals,
        LengthConstraintLessThan,
        LengthConstraingGreaterThan
    };

    enum ConstraintClassification {
        ConstraintClassificationAntimonotone,
        ConstraintClassificationMonotone,
        ConstraintClassificationSuccint,
        ConcstraintClassificationConvertible
    };

    // For each constraint type (ItemConstraintType, key of QHash), we allow
    // multiple entries (QVector), each of which is a set of items (QSet<ItemName>)
    // that are OR'd together (positive: (a == x1 OR b == x1 OR ... OR n == x1);
    // negative: !(a == x1 OR b == x1 OR ... OR n == x1)), while the entries
    // (QVector) are AND'ed together.
    typedef QHash<ItemConstraintType, QVector<QSet<ItemName> > > ItemConstraintsHash;

    class Constraints {

#ifdef DEBUG
        friend QDebug operator<<(QDebug dbg, const Constraints & constraints);
#endif

    public:
        Constraints();

        void reset();
        bool empty() const { return this->itemConstraints.empty(); }

        void addItemConstraint(const QSet<ItemName> & items, ItemConstraintType type);
        void setItemConstraints(const ItemConstraintsHash & itemConstraints);

        QSet<ItemID> getAllItemIDsForConstraintType(ItemConstraintType type) const;

        void preprocessItemIDNameHash(const ItemIDNameHash & hash);

        void preprocessItem(const ItemName & name, ItemID id);
        void removeItem(ItemID id);
        ItemID getHighestPreprocessedItemID() const { return this->highestPreprocessedItemID; }
        void clearPreprocessedItems() { this->preprocessedItemConstraints.clear(); this->highestPreprocessedItemID = ROOT_ITEMID; }

        bool matchItemset(const ItemIDList & itemset,
                          const QSet<ConstraintClassification> & constraintsSubset = QSet<ConstraintClassification>()
                          ) const;
        bool matchItemset(const QSet<ItemName> & itemset,
                          const QSet<ConstraintClassification> & constraintsSubset = QSet<ConstraintClassification>()
                          ) const;
        bool matchSearchSpace(const ItemIDList & frequentItemset, const QHash<ItemID, SupportCount> & prefixPathsSupportCounts) const;

#ifdef DEBUG
        ItemIDNameHash * itemIDNameHash;
#endif

        static const char * ItemConstraintTypeName[2];
        static const char * LengthConstraintTypeName[3];
        static const ConstraintClassification ItemConstraintTypeClassification[2];
        static const ConstraintClassification LengthConstraintTypeClassification[3];

    protected:
        static bool matchItemsetHelper(const ItemIDList & itemset, ItemConstraintType type, const QSet<ItemID> & constraintItems);
        static bool matchItemsetHelper(const QSet<ItemName> & itemset, ItemConstraintType type, const QSet<ItemName> & constraintItems);
        static bool matchSearchSpaceHelper(const ItemIDList & frequentItemset, const QHash<ItemID, SupportCount> & prefixPathsSupportCounts, ItemConstraintType type, const QSet<ItemID> & constraintItems);

        void addPreprocessedItemConstraint(ItemConstraintType type, uint constraint, ItemID id);

        ItemConstraintsHash itemConstraints;
        QHash<ItemConstraintType, QVector<QSet<ItemID> > > preprocessedItemConstraints;
        ItemID highestPreprocessedItemID;
    };
}

#endif // CONSTRAINTS_H
