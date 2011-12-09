#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <QtGlobal>
#include <QMetaType>
#include <QList>
#include <QStringList>
#include <QString>
#include <QHash>
#include <QMetaType>
#include <QDebug>

#include "../common/common.h"


namespace Config {


// Efficient storage of Episode names: don't store the actual names, use
// 8-bit IDs instead. This allows for 256 different Episode names, which
// should be more than sufficient.
typedef QString EpisodeName;
typedef quint8 EpisodeID;
typedef QHash<EpisodeName, EpisodeID> EpisodeNameIDHash;
typedef QHash<EpisodeID, EpisodeName> EpisodeIDNameHash;
// Store Episode durations as 32-bit uints.
typedef quint32 EpisodeDuration;
// The EpisodeDuration will be discretized to an EpisodeSpeed for association
// rule mining.
typedef QString EpisodeSpeed;
struct Episode {
    Episode() {}
    Episode(EpisodeID id, EpisodeDuration duration) : id(id), duration(duration) {}

    EpisodeID id;
    EpisodeDuration duration;
#ifdef DEBUG
    EpisodeIDNameHash * IDNameHash;
#endif
};
inline bool operator==(const Episode &e1, const Episode &e2) {
    return e1.id == e2.id && e1.duration == e2.duration;
}
typedef QList<Episode> EpisodeList;

// Efficient storage of circumstances.
typedef QString Circumstance;
typedef QSet<Circumstance> Circumstances;

// Parsed sample.
struct Sample {
    Time time;
    EpisodeList episodes;
    Circumstances circumstances;
#ifdef DEBUG
    EpisodeIDNameHash * episodeIDNameHash;
#endif
};

void registerMetaTypes();

#ifdef DEBUG
// QDebug() streaming output operators.
QDebug operator<<(QDebug dbg, const Episode & episode);
QDebug operator<<(QDebug dbg, const Episode & episodeList);
QDebug operator<<(QDebug dbg, const Circumstances & circumstances);
QDebug operator<<(QDebug dbg, const Sample & sample);
#endif


}

#endif // TYPEDEFS_H
