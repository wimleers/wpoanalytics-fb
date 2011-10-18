#include "Item.h"

namespace Analytics {

    /**
     * It's necessary to register the metatypes defined in Item.h to allow
     * these types to be used in queued signal/slot connections, for example.
     *
     * An extra difficulty is the combination with namespaces. See
     * http://ktutorial.wordpress.com/2009/04/26/qt-meta-object-system-and-namespaces/
     * and http://lists.trolltech.com/qt-interest/2007-11/thread00465-0.html
     */
    void registerBasicMetaTypes() {
        qRegisterMetaType<Analytics::SupportCount>("Analytics::SupportCount");
        qRegisterMetaType<Analytics::Confidence>("Analytics::Confidence");
        qRegisterMetaType<Analytics::ItemIDList>("ItemIDList");
        qRegisterMetaType<Analytics::FrequentItemset>("FrequentItemset");
        qRegisterMetaType< QList<Analytics::Confidence> >("QList<Analytics::Confidence>");
        qRegisterMetaType< QList<Analytics::AssociationRule> >("QList<Analytics::AssociationRule>");
        qRegisterMetaType<Analytics::ItemIDNameHash>("Analytics::ItemIDNameHash");
    }

    uint qHash(const AssociationRule & r) {
        QString s;
        foreach (const ItemID & id, r.antecedent)
            s += QString::number(id) + ':';
        s += "=>";
        foreach (const ItemID & id, r.consequent)
            s += QString::number(id) + ':';
        return qHash(s);
    }

#ifdef DEBUG
    QDebug operator<<(QDebug dbg, const Item & i) {
        dbg.nospace() << i.IDNameHash->value(i.id).toStdString().c_str()
                << "("
                << i.id
                << ")="
                << i.supportCount;

        return dbg.nospace();
    }

    QDebug operator<<(QDebug dbg, const ItemIDList & pattern) {
        dbg.nospace() << "{";

        for (int i = 0; i < pattern.size(); i++) {
            if (i > 0)
                dbg.nospace() << ", ";

            dbg.nospace() << pattern[i];
        }
        dbg.nospace() << "}";

        return dbg.nospace();
    }

    QDebug operator<<(QDebug dbg, const Transaction & transaction) {
        QString itemOutput;

        dbg.nospace() << "[size=" << transaction.size() << "] {";

        for (int i = 0; i < transaction.size(); i++) {
            if (i > 0)
                dbg.nospace() << ", ";

            // Generate output for item.
            itemOutput.clear();
            QDebug(&itemOutput) << transaction[i];

            dbg.nospace() << itemOutput.toStdString().c_str();
        }
        dbg.nospace() << "}";

        return dbg.nospace();
    }

    QDebug operator<<(QDebug dbg, const FrequentItemset & frequentItemset) {
        dbg.nospace() << "({";
        itemIDHelper(dbg, frequentItemset.itemset, frequentItemset.IDNameHash);
        dbg.nospace() << "}, sup: "
                      << frequentItemset.support
                      << ")";

        return dbg.nospace();
    }

    QDebug operator<<(QDebug dbg, const AssociationRule & associationRule) {
        dbg.nospace() << "{";
        itemIDHelper(dbg, associationRule.antecedent, associationRule.IDNameHash);
        dbg.nospace() << "} => {";
        itemIDHelper(dbg, associationRule.consequent, associationRule.IDNameHash);
        dbg.nospace() << "}";

        dbg.nospace() << " ("
                      << "sup=" << associationRule.support
                      << ", conf=" << associationRule.confidence
                      << ")";

        return dbg.nospace();
    }

    QDebug itemIDHelper(QDebug dbg, const ItemIDList & itemset, ItemIDNameHash const * const IDNameHash) {
        for (int i = 0; i < itemset.size(); i++) {
            if (i > 0)
                dbg.nospace() << ", ";

            if (IDNameHash != NULL) {
                dbg.nospace() << IDNameHash->value(itemset[i]).toStdString().c_str()
                        << "("
                        << itemset[i]
                        << ")";
            }
            else
                dbg.nospace() << itemset[i];
        }

        return dbg.nospace();
    }

#endif

}
