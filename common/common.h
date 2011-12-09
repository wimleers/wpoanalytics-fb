#ifndef COMMON_H
#define COMMON_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QMetaType>

typedef quint32 Time;
typedef QStringList RawTransaction;
// TiltedTimeWindow.
typedef int Granularity;
typedef int Bucket;

struct BatchMetadata {
    BatchMetadata() : discardedSamples(0.0) {}

    void setChunkInfo(quint32 batchID,
                      bool isLastChunk,
                      quint32 numDiscardedSamples)
    {
        this->batchID             = batchID;
        this->isLastChunk         = isLastChunk;
        this->discardedSamples = numDiscardedSamples;
    }

    // Time perspective.
    quint32 batchID;
    Time startTime;
    Time endTime;
    bool isLastChunk;

    // Stats.
    quint32 samples;
    quint32 transactions;
    quint32 items;
    double transactionsPerSample;
    double itemsPerTransaction;
    quint32 discardedSamples;
};

// Call with either RawTransaction or Config::Sample.
template <class T>
struct Batch {
    QList<T> data;
    BatchMetadata meta;
};

void registerCommonMetaTypes();

#endif // COMMON_H
