#include "ConceptHierarchyModel.h"

ConceptHierarchyModel::ConceptHierarchyModel() {
    this->reset();
}

void ConceptHierarchyModel::update(Analytics::ItemIDNameHash itemIDNameHash) {
    if (itemIDNameHash.size() <= this->itemsAlreadyProcessed)
        return;

    Analytics::ItemName item, parent, child;
    QStringList parts;
    for (int id = this->itemsAlreadyProcessed; id < itemIDNameHash.size(); id++) {
        item = itemIDNameHash[(Analytics::ItemID) id];
        parts = item.split(':', QString::SkipEmptyParts);

        // Update the concept hierarchy model.
        parent = parts[0];
        // Root level.
        if (!this->hash.contains(parent)) {
            QStandardItem * modelItem = new QStandardItem(parent);
            modelItem->setData(parent.toUpper(), Qt::UserRole); // For sorting.

            // Store in hierarchy.
            this->hash.insert(parent, modelItem);
            QStandardItem * root = this->invisibleRootItem();
            root->appendRow(modelItem);
        }
        // Subsequent levels.
        for (int p = 1; p < parts.size(); p++) {
            child = parent + ':' + parts[p];
            if (!this->hash.contains(child)) {
                QStandardItem * modelItem = new QStandardItem(parts[p]);
                modelItem->setData(parts[p].toUpper(), Qt::UserRole); // For sorting.

                // Store in hierarchy.
                this->hash.insert(child, modelItem);
                QStandardItem * parentModelItem = this->hash[parent];
                parentModelItem->appendRow(modelItem);
            }
            parent = child;
        }
    }

    // Remember the last item processed.
    this->itemsAlreadyProcessed = itemIDNameHash.size();

    // Sort the model case-insensitively.
    this->setSortRole(Qt::UserRole);
    this->sort(0, Qt::AscendingOrder);
}

void ConceptHierarchyModel::reset() {
    this->itemsAlreadyProcessed = 0;
    this->hash.clear();
}
