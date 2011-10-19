#ifndef PARSER_H
#define PARSER_H

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


namespace FacebookLogParser {

    #define PARSE_CHUNK_SIZE 4000
    #define PROCESS_CHUNK_SIZE 50000
    #define CHECK_TIME_INTERVAL 100

    class Parser : public QObject {
        Q_OBJECT

    public:
        Parser(const Config::Config * const config);

        // Processing logic.
        static Config::Sample parseSample(const QString & rawSample, const Config::Config * const config);
        static QList<QStringList> mapSampleToTransactions(const Config::Sample & sample, const Config::Config * const config);

    signals:
        void parsing(bool);
        void parsedDuration(int duration);
        void parsedBatch(QList<QStringList> transactions, double transactionsPerEvent, Time start, Time end, quint32 quarterID, bool lastChunkOfBatch);

    public slots:
        void parse(const QString & fileName);
        void continueParsing();

    protected slots:
        void processBatch(const QList<Config::Sample> batch, quint32 quarterID, bool lastChunkOfBatch);

    protected:
        void processParsedChunk(const QStringList & chunk, bool forceProcessing = false);
        static quint32 calculateQuarterID(Time t);

        QMutex mutex;
        QWaitCondition condition;
        QTime timer;


        // QHashes that are used to minimize memory usage.
        static Config::EpisodeNameIDHash episodeNameIDHash;
        static Config::EpisodeIDNameHash episodeIDNameHash;
        static QHash<Config::EpisodeName, QString> episodeNameFieldNameHash;

        const Config::Config * config;

        // Mutexes used to ensure thread-safety.
        static QMutex episodeHashMutex;

        // Methods to actually use the above QHashes.
        static Config::EpisodeID mapEpisodeNameToID(const Config::EpisodeName & name, const QString & fieldName);
    };

}
#endif // PARSER_H
