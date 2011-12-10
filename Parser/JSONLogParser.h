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

#include "../shared/qxtjson.h"
#include "../common/common.h"
#include "../Config/Config.h"


enum WindowMarkerMethod {
    WindowMarkerMethodMarkerLine, // Batching during parsing.
    WindowMarkerMethodTimestamp   // Batching during processing.
};

namespace JSONLogParser {

    #define PARSE_CHUNK_SIZE 4000
    #define PROCESS_CHUNK_SIZE 50000
    #define CHECK_TIME_INTERVAL 100

    class Parser : public QObject {
        Q_OBJECT

    public:
        Parser(Config::Config config, uint secPerBatch);

        // Processing logic.
        static Config::Sample parseSample(const QString & rawSample, const Config::Config * const config);
        static QList<QStringList> mapSampleToTransactions(const Config::Sample & sample, const Config::Config * const config);

    signals:
        void parsing(bool);
        void stats(int duration, BatchMetadata metadata);
        void parsedChunkOfBatch(Batch<RawTransaction> transactions);

    public slots:
        void parse(const QString & fileName);
        void continueParsing();

    protected slots:
        void processChunkOfBatch(const Batch<Config::Sample> & chunk);

    protected:
        virtual WindowMarkerMethod getWindowMarkerMethod() const;
        virtual QString getWindowMarkerLine() const;
        virtual quint32 calculateBatchID(Time t);
        virtual void processParsedChunk(const QStringList & chunk,
                                        bool finishesTimeWindow,
                                        bool forceProcessing = false);

        static Config::EpisodeID mapEpisodeNameToID(const Config::EpisodeName & name, const QString & fieldName);

        // Parameters.
        uint secPerBatch;
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

    struct ParseSampleMapper {
        ParseSampleMapper(const Config::Config * const config) : config(config) {}

        typedef Config::Sample result_type;

        Config::Sample operator()(const QString & rawSample) {
            return Parser::parseSample(rawSample, config);
        }

        const Config::Config * const config;
    };

    struct SampleToTransactionMapper {
        SampleToTransactionMapper(const Config::Config * const config) : config(config) {}

        typedef QList<QStringList> result_type;

        QList<QStringList> operator()(const Config::Sample & sample) {
            return Parser::mapSampleToTransactions(sample, config);
        }

        const Config::Config * const config;
    };
}
#endif // JSONLOGPARSER_H
