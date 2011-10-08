#ifndef CAUSESTABLEFILTERPROXYMODEL_H
#define CAUSESTABLEFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QDebug>

class CausesTableFilterProxyModel : public QSortFilterProxyModel {

    Q_OBJECT

public:
    explicit CausesTableFilterProxyModel(QObject * parent = NULL);

    void setEpisodesColumn(int col);
    void setCircumstancesColumn(int col);
    void setConsequentsColumn(int col);

    void setEpisodeFilter(const QString & filter);
    void setCircumstancesFilter(const QStringList & filter);
    void setConsequentsFilter(const QStringList & filter);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex & sourceParent) const;

private:
    int episodesColumn;
    int circumstancesColumn;
    int consequentsColumn;
    QRegExp episodesFilter;
    QList<QRegExp> circumstancesFilter;
    QList<QRegExp> consequentsFilter;
};

#endif // CAUSESTABLEFILTERPROXYMODEL_H
