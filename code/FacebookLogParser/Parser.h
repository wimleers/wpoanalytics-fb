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

#include "EpisodeDurationDiscretizer.h"
#include "typedefs.h"


namespace FacebookLogParser {

    #define CHUNK_SIZE 4000

    class Parser : public QObject {
        Q_OBJECT

    public:
        Parser();
        static void initParserHelpers(const QString & discretizerCSV);
        static void clearParserHelperCaches();

        // Processing logic.
        static Sample parseSample(const QString & rawSample);
        static QList<QStringList> mapSampleToTransactions(const Sample & sample);

    signals:
        void parsing(bool);
        void parsedDuration(int duration);
        void parsedBatch(QList<QStringList> transactions, double transactionsPerEvent, Time start, Time end);

    public slots:
        void parse(const QString & fileName);
        void continueParsing();

    protected slots:
        void processBatch(const QList<Sample> batch);

    protected:
        void processParsedChunk(const QStringList & chunk, bool forceProcessing = false);

        QMutex mutex;
        QWaitCondition condition;
        QTime timer;


        // QHashes that are used to minimize memory usage.
        static EpisodeNameIDHash episodeNameIDHash;
        static EpisodeIDNameHash episodeIDNameHash;

        static bool parserHelpersInitialized;
        static EpisodeDurationDiscretizer episodeDiscretizer;

        // Mutexes used to ensure thread-safety.
        static QMutex parserHelpersInitMutex;
        static QMutex episodeHashMutex;

        // Methods to actually use the above QHashes.
        static EpisodeID mapEpisodeNameToID(EpisodeName name);
    };

}
#endif // PARSER_H
