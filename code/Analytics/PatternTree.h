#ifndef PATTERNTREE_H
#define PATTERNTREE_H

#include <QDebug>
#include <QMetaType>
#include <QFile>
#include <QTextStream>

#include "Item.h"
#include "TiltedTimeWindow.h"
#include "FPNode.h"
#include "Constraints.h"

#include "qxtjson.h"


namespace Analytics {
    class PatternTree {
    public:
        PatternTree();
        ~PatternTree();
        void setTTWDefinition(const TTWDefinition & ttwDef) {
            this->ttwDef = ttwDef;

            // Update the root node with the TTW definition.
            this->root->getPointerToValue()->build(this->ttwDef);
        }
        const TTWDefinition & getTTWDefinition() const { return this->ttwDef; }

        bool serialize(QTextStream & output,
                                   const ItemIDNameHash & itemIDNameHash) const;
        bool deserialize(QTextStream & input,
                                       const ItemIDNameHash * itemIDNameHash,
                                       const ItemNameIDHash & itemNameIDHash,
                                       quint32 updateID);

        // Accessors.
        FPNode<TiltedTimeWindow> * getRoot() const { return this->root; }
        TiltedTimeWindow * getPatternSupport(const ItemIDList & pattern) const;
        unsigned int getNodeCount() const { return this->nodeCount; }
        uint getCurrentQuarter() const { return this->currentQuarter; }
        QList<FrequentItemset> getFrequentItemsetsForRange(SupportCount minSupport,
                                                           const Constraints & frequentItemsetConstraints,
                                                           uint from,
                                                           uint to,
                                                           const ItemIDList & prefix = ItemIDList(),
                                                           const FPNode<TiltedTimeWindow> * node = NULL) const;
        SupportCount getTotalSupportForRange(const Constraints & c,
                                             uint from, uint to) const;

        // Modifiers.
        void addPattern(const FrequentItemset & pattern, quint32 updateID);
        void removePattern(FPNode<TiltedTimeWindow> * const node);
        void nextQuarter() { this->currentQuarter = (currentQuarter + 1) % 4; }

        // Static (class) methods.
        static ItemIDList getPatternForNode(FPNode<TiltedTimeWindow> const * const node);

    protected:
        // Static (class) methods.
        static void recursiveSerializer(const FPNode<TiltedTimeWindow> * node,
                                        const ItemIDNameHash & itemIDNameHash,
                                        QTextStream & output,
                                        QList<ItemName> pattern);
        static bool totalSupportForRangeHelper(const Constraints & c,
                                               uint from, uint to,
                                               uint & totalSupport,
                                               const ItemIDList & pattern,
                                               const FPNode<TiltedTimeWindow> * node);


        TTWDefinition ttwDef;
        FPNode<TiltedTimeWindow> * root;
        uint currentQuarter;
        unsigned int nodeCount;
    };

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const PatternTree & tree);
    QString dumpHelper(const FPNode<TiltedTimeWindow> & node, QString prefix = "");

    // QDebug output operators for FPNode<TiltedTimeWindow>.
    QDebug operator<<(QDebug dbg, const FPNode<TiltedTimeWindow> & node);
#endif

}

Q_DECLARE_METATYPE(Analytics::PatternTree);

#endif // PATTERNTREE_H
