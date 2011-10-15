#ifndef CONCEPTHIERARCHYMODEL_H
#define CONCEPTHIERARCHYMODEL_H

#include <QStandardItem>
#include <QStandardItemModel>
#include <QHash>

#include "../Analytics/Item.h"

class ConceptHierarchyModel : public QStandardItemModel {
    Q_OBJECT

public:
    ConceptHierarchyModel();

public slots:
    void update(Analytics::ItemIDNameHash itemIDNameHash);
    void reset();

private:
    int itemsAlreadyProcessed;
    QHash<Analytics::ItemName, QStandardItem *> hash;
};

#endif // CONCEPTHIERARCHYMODEL_H
