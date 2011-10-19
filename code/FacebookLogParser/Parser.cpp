#include "Parser.h"

namespace FacebookLogParser {
    Config::EpisodeNameIDHash Parser::episodeNameIDHash;
    Config::EpisodeIDNameHash Parser::episodeIDNameHash;
    QHash<Config::EpisodeName, QString> Parser::episodeNameFieldNameHash;

    QMutex Parser::episodeHashMutex;

    Parser::Parser(const Config::Config * const config) {
        this->config = config;
    }


    //---------------------------------------------------------------------------
    // Protected methods.


    /**
     * Parse the given Episodes log file.
     *
     * Emits a signal for every chunk (with PARSE_CHUNK_SIZE lines). This signal
     * can then be used to process that chunk. This allows multiple chunks to be
     * processed in parallel (by using QtConcurrent), if that is desired.
     *
     * @param fileName
     *   The full path to an Episodes log file.
     */
    void Parser::parse(const QString & fileName) {
        // Notify the UI.
        emit parsing(true);

        QFile file;
        QStringList chunk;
        int numLines = 0;

        file.setFileName(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Could not open file for reading.";
            // TODO: emit signal indicating parsing failure.
            return;
        }
        else {
            this->timer.start();
            QTextStream in(&file);
            while (!in.atEnd()) {
                chunk.append(in.readLine());
                numLines++;
                if (chunk.size() == PARSE_CHUNK_SIZE) {
                    this->processParsedChunk(chunk);
                    chunk.clear();
                }
            }

            // Check if we have another chunk (with size < PARSE_CHUNK_SIZE).
            if (chunk.size() > 0) {
                // TODO: remove the forceProcessing = true parameter!
                this->processParsedChunk(chunk, true);
            }

            // Notify the UI.
            emit parsing(false);
        }
    }

    void Parser::continueParsing() {
        QMutexLocker(&this->mutex);
        this->condition.wakeOne();
    }


    //---------------------------------------------------------------------------
    // Protected methods.

    /**
     * Map an episode name to an episode ID. Generate a new ID when necessary.
     *
     * @param name
     *   Episode name.
     * @param fieldName
     *   The name of the field; this may differ from the episode name sometimes.
     * @return
     *   The corresponding episode ID.
     *
     * Modifies a class variable upon some calls (i.e. when a new key must be
     * inserted in the QHash), hence we need to use a mutex to ensure thread
     * safety.
     */
    Config::EpisodeID Parser::mapEpisodeNameToID(const Config::EpisodeName & name, const QString & fieldName) {
        if (!Parser::episodeNameIDHash.contains(name)) {
            Parser::episodeHashMutex.lock();

            Config::EpisodeID id = Parser::episodeNameIDHash.size();
            Parser::episodeNameIDHash.insert(name, id);
            Parser::episodeIDNameHash.insert(id, name);

            Parser::episodeNameFieldNameHash.insert(name, fieldName);

            Parser::episodeHashMutex.unlock();
        }

        return Parser::episodeNameIDHash[name];
    }

    /**
     * Map a sample (raw string) to a Sample data structure.
     *
     * @param sample
     *   Raw sample, as read from .csv/Scribe/RockfortExpress.
     * @return
     *   Corresponding Sample data structure.
     */
    Config::Sample Parser::parseSample(const QString & rawSample, const Config::Config * const config) {
        Config::Sample sample;

        // Get config.
        QHash<Config::EpisodeName, Config::Attribute> numericalAttributes = config->getNumericalAttributes();
        QHash<Config::EpisodeName, Config::Attribute> categoricalAttributes = config->getCategoricalAttributes();
        Config::Attribute attribute;

        // Parse.
        QVariantMap json = QxtJSON::parse(rawSample).toMap();

        // 1) Process normals.
        const QVariantMap & normals = json["normal"].toMap();
        foreach (const QString & key, normals.keys()) {
            if (categoricalAttributes.contains(key)) {
                attribute = categoricalAttributes[key];

                if (attribute.parentAttribute.isNull())
                    sample.circumstances.insert(attribute.name + ':' + normals[key].toString());
                else if (!normals.contains(attribute.parentAttribute)) {
                    qWarning("A sample did NOT contain the parent attribute '%s' for the attribute '%s'!", qPrintable(attribute.parentAttribute), qPrintable(attribute.name));
                    sample.circumstances.insert(attribute.name + ':' + normals[attribute.parentAttribute].toString() + ':' + normals[key].toString());
                }
            }
        }

        // 2) Process denormals.
        const QVariantMap & denorms = json["denorm"].toMap();
        foreach (const QString & key, denorms.keys()) {
            if (categoricalAttributes.contains(key)) {
                attribute = categoricalAttributes[key];
                sample.circumstances.insert(attribute.name + ":" + denorms[key].toString());
            }
        }

        Config::Circumstances categoricalCircumstances = sample.circumstances;

        // 3) Process ints.
        const QVariantMap & integers = json["int"].toMap();
        Config::Episode episode;
        Config::EpisodeSpeed speed;
        foreach (const QString & key, integers.keys()) {
            if (numericalAttributes.contains(key)) {
                attribute = numericalAttributes[key];

                if (attribute.isEpisode) {
                    episode.id = Parser::mapEpisodeNameToID(attribute.name, attribute.field);
                    episode.duration = integers[key].toInt();
#ifdef DEBUG
                    episode.IDNameHash = &Parser::episodeIDNameHash;
#endif
                    // Drop the episode when no data was collected.
                    if (episode.duration > 0)
                        sample.episodes.append(episode);
                }
                else {
                    // Discretize based on the circumstances gathered from the
                    // categorical attributes. This ensures that all numerical
                    // attributes that follow this code path will receive an
                    // identical set of circumstances.
                    speed = config->discretize(attribute.field, integers[key].toInt(), categoricalCircumstances);
                    sample.circumstances.insert(attribute.name + ":" + speed);
                }
            }
        }
        // One special case: time.
        sample.time = integers["time"].toInt();

        return sample;
    }

    QList<QStringList> Parser::mapSampleToTransactions(const Config::Sample & sample, const Config::Config * const config) {
        QList<QStringList> transactions;

        Config::Episode episode;
        Config::EpisodeName episodeName;
        QStringList transaction;
        foreach (episode, sample.episodes) {
            episodeName = Parser::episodeIDNameHash[episode.id];
            transaction << QString("episode:") + episodeName
                        << QString("duration:") + config->discretize(
                               Parser::episodeNameFieldNameHash[episodeName],
                               episode.duration,
                               sample.circumstances
                           )
                        // Append the circumstances.
                        << sample.circumstances.toList();
            transactions << transaction;
            transaction.clear();
        }

        return transactions;
    }


    //---------------------------------------------------------------------------
    // Protected slots.

    void Parser::processBatch(const QList<Config::Sample> batch, quint32 quarterID, bool lastChunkOfBatch) {
        double transactionsPerEvent;
#ifdef DEBUG
        uint items = 0;
#endif

        // This 100% concurrent approach fails, because QGeoIP still has
        // thread-safety issues. Hence, we only do the mapping from a QString
        // to an EpisodesLogLine concurrently for now.
        // QtConcurrent::blockingMapped(chunk, Parser::mapAndExpandToEpisodesLogLine);

        // Perform the mapping from ExpandedEpisodesLogLines to groups of
        // transactions concurrently
//        QList< QList<QStringList> > groupedTransactions = QtConcurrent::blockingMapped(expandedChunk, Parser::mapExpandedEpisodesLogLineToTransactions);
        QList< QList<QStringList> > groupedTransactions;
        Config::Sample sample;
        foreach (sample, batch) {
            groupedTransactions << Parser::mapSampleToTransactions(sample, this->config);
        }

        // Perform the merging of transaction groups into a single list of
        // transactions sequentially (impossible to do concurrently).
        QList<QStringList> transactions;
        QList<QStringList> transactionGroup;
        foreach (transactionGroup, groupedTransactions) {
            transactions.append(transactionGroup);
#ifdef DEBUG
            foreach (const QStringList & transaction, transactionGroup)
                items += transaction.size();
#endif
        }

        transactionsPerEvent = ((double) transactions.size()) / batch.size();

        qDebug() << "Processed batch of" << batch.size() << "lines!"
                 << "(Last chunk?" << (lastChunkOfBatch ? "Yes" : "No") << ")"
                 << "Transactions generated:" << transactions.size() << "."
                 << "(" << transactionsPerEvent << "transactions/event)"
#ifdef DEBUG
                 << "Avg. transaction length:" << 1.0 * items / transactions.size() << "."
                 << "(" << items << "items in total)"
#endif
                 << "Events occurred between"
                 << QDateTime::fromTime_t(batch.first().time).toString("yyyy-MM-dd hh:mm:ss").toStdString().c_str()
                 << "and"
                 << QDateTime::fromTime_t(batch.last().time).toString("yyyy-MM-dd hh:mm:ss").toStdString().c_str();
        emit parsedDuration(timer.elapsed());
        emit parsedBatch(transactions, transactionsPerEvent, batch.first().time, batch.last().time, quarterID, lastChunkOfBatch);


        // Pause the parsing until these transactions have been processed!
        this->mutex.lock();
        this->condition.wait(&this->mutex);
        this->timer.start(); // Restart the timer.
        this->mutex.unlock();
    }


    //---------------------------------------------------------------------------
    // Protected methods.

    void Parser::processParsedChunk(const QStringList & chunk, bool forceProcessing) {
        static quint32 currentQuarterID = 0;
        static QList<Config::Sample> batch;
        quint16 sampleNumber = 0;

        // Perform the mapping from strings to EpisodesLogLine concurrently.
//        QList<EpisodesLogLine> mappedChunk = QtConcurrent::blockingMapped(chunk, Parser::mapLineToEpisodesLogLine);
        QString rawSample;
        Config::Sample sample;
        foreach (rawSample, chunk) {
            // Don't waste time parsing checkpointing lines.
            if (rawSample.at(0) == '#')
                continue;

            sample = Parser::parseSample(rawSample, this->config);

            // Calculate the initial currentQuarterID.
            if (currentQuarterID == 0)
                currentQuarterID = Parser::calculateQuarterID(sample.time);

            sampleNumber++;

            // Create a batch for each quarter (900 seconds) and process it.
            if (sampleNumber % CHECK_TIME_INTERVAL == 0) { // Only check once very CHECK_TIME_INTERVAL lines.
                sampleNumber = 0; // Reset.
                quint32 quarterID = Parser::calculateQuarterID(sample.time);
                if (quarterID > currentQuarterID && !batch.isEmpty()) {
                    currentQuarterID = quarterID;

                    // The batch we just finished is the previous quarter ID!
                    this->processBatch(batch, quarterID - 1, true);
                    batch.clear();
                }
            }

            // Ensure that the batch doesn't get too large (and thus consumes
            // too much memory): let it be processed when it has grown to
            // PROCESS_CHUNK_SIZE lines.
            if (batch.size() == PROCESS_CHUNK_SIZE) {
                this->processBatch(batch, currentQuarterID, false); // The batch doesn't end here!
                batch.clear();
            }

            batch.append(sample);
        }

        if (forceProcessing && !batch.isEmpty()) {
            this->processBatch(batch, currentQuarterID, true);
            batch.clear();
        }
    }

    quint32 Parser::calculateQuarterID(Time t) {
        static quint32 minQuarterID = 0;

        quint32 quarterID;

        quarterID = t / 900;

        // Cope with data that is not chronologically ordered perfectly.
        if (quarterID > minQuarterID)
            minQuarterID = quarterID;
        else if (minQuarterID > quarterID)
            quarterID = minQuarterID;

        return quarterID;
    }
}
