#ifndef JSONLOGPARSER_H
#define JSONLOGPARSER_H

#include <QObject>
#include <QFile>
#include <QRegExp>
#include <QTextStream>
#include <QStringList>
#include <QThread>
#include <QtConcurrentMap>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QTime>
#include <QVariant>

#include "qxtjson.h"


#include "../Config/Config.h"


typedef uint Time;


namespace JSONLogParser {

    #define PARSE_CHUNK_SIZE 4000
    #define PROCESS_CHUNK_SIZE 50000
    #define CHECK_TIME_INTERVAL 100

    class Parser : public QObject {
        Q_OBJECT

    public:
        Parser(Config::Config config);

        // Processing logic.
        static Config::Sample parseSample(const QString & rawSample, const Config::Config * const config);

    signals:
        void parsing(bool);
        void stats(int duration, quint64 transactions, double transactionsPerEvent, double averageTransactionLength, bool lastChunkOfBatch, Time start, Time end);
        void parsedBatch(QList<QStringList> transactions, double transactionsPerEvent, Time start, Time end, quint32 quarterID, bool lastChunkOfBatch);

    public slots:
        void parse(const QString & fileName);
        void continueParsing();

    protected slots:
        void processBatch(const QList<Config::Sample> batch, quint32 quarterID, bool lastChunkOfBatch);

    protected:
        virtual void processParsedChunk(const QStringList & chunk, bool forceProcessing = false);

        static QList<QStringList> mapSampleToTransactions(const Config::Sample & sample, const Config::Config * const config);
        static Config::EpisodeID mapEpisodeNameToID(const Config::EpisodeName & name, const QString & fieldName);
        static quint32 calculateQuarterID(Time t);

        Config::Config config; // No const reference/pointer because we need a
                               // copy: the Parser runs in its own thread.

        QMutex mutex;
        QWaitCondition condition;
        QTime timer;

        // QHashes that are used to minimize memory usage.
        static QMutex episodeHashMutex;
        static Config::EpisodeNameIDHash episodeNameIDHash;
        static Config::EpisodeIDNameHash episodeIDNameHash;
        static QHash<Config::EpisodeName, QString> episodeNameFieldNameHash;
    };

}
#endif // JSONLOGPARSER_H
