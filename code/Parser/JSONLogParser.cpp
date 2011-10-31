#include "Parser.h"

namespace JSONLogParser {
    Config::EpisodeNameIDHash Parser::episodeNameIDHash;
    Config::EpisodeIDNameHash Parser::episodeIDNameHash;
    QHash<Config::EpisodeName, QString> Parser::episodeNameFieldNameHash;

    QMutex Parser::episodeHashMutex;

    Parser::Parser(Config::Config config) {
        this->config = config;
    }

    //---------------------------------------------------------------------------
    // Public static methods.


    /**
     * Map a sample (raw string) to a Sample data structure.
     *
     * @param sample
     *   Raw sample, a JSON object with 'int', 'normal', 'denorm' keys, each of
     *   which maps to an array of single-level objects.
     * @return
     *   Corresponding Sample data structure.
     */
    Config::Sample Parser::parseSample(const QString & rawSample, const Config::Config * const config) {
        Config::Sample sample;
        Analytics::Constraints constraints;

        // Get config.
        constraints.setItemConstraints(config->getParserItemConstraints());
        QHash<Config::EpisodeName, Config::Attribute> numericalAttributes = config->getNumericalAttributes();
        QHash<Config::EpisodeName, Config::Attribute> categoricalAttributes = config->getCategoricalAttributes();
        Config::Attribute attribute;

        // Parse.
        QVariantMap json = QxtJSON::parse(rawSample).toMap();

        // 1) Process normals.
        const QVariantMap & normals = json["normal"].toMap();
        QString value;
        foreach (const QString & key, normals.keys()) {
            if (categoricalAttributes.contains(key)) {
                attribute = categoricalAttributes[key];

                value = normals[key].toString();

                // parentAttribute: only insert a circumstance if the parent
                // attribute also exists.
                if (!attribute.parentAttribute.isNull()) {
                    if (normals.contains(attribute.parentAttribute)) {
                        sample.circumstances.insert(
                            attribute.name +
                            ':' +
                            normals[attribute.parentAttribute].toString() +
                            ':' +
                            value
                        );
                    }
                    else {
                        qWarning(
                            "A sample did NOT contain the parent attribute '%s' for the attribute '%s'!",
                            qPrintable(attribute.parentAttribute),
                            qPrintable(attribute.name)
                        );
                    }
                }
                // hierarchySeparator: insert multiple items if the hierarchy
                // separator does exist, otherwise just insert the circumstance
                // itself.
                else if (!attribute.hierarchySeparator.isNull() && value.contains(attribute.hierarchySeparator)) {
                    // Insert all sections.
                    for (int i = 0; i < value.count(attribute.hierarchySeparator, Qt::CaseSensitive); i++) {
                        sample.circumstances.insert(
                            attribute.name +
                            value.section(attribute.hierarchySeparator, 0, i)
                        );
                    }
                    // Insert the whole thing, too.
                    sample.circumstances.insert(attribute.name + ':' + value);
                }
                else
                    sample.circumstances.insert(attribute.name + ':' + value);
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

        // If the sample doesn't match the constraints, clear the circumstances
        // and return that right away. (Samples without circumstances are
        // discarded.)
        if (!constraints.matchItemset(sample.circumstances)) {
            sample.circumstances.clear();
            return sample;
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


    //---------------------------------------------------------------------------
    // Public slots.

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
    // Protected slots.

    void Parser::processBatch(const QList<Config::Sample> batch,
                              quint32 quarterID,
                              bool lastChunkOfBatch,
                              quint32 discardedSamples)
    {
        double transactionsPerEvent;
        uint items = 0;
        double averageTransactionLength;

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
            groupedTransactions << Parser::mapSampleToTransactions(sample, &this->config);
        }

        // Perform the merging of transaction groups into a single list of
        // transactions sequentially (impossible to do concurrently).
        QList<QStringList> transactions;
        QList<QStringList> transactionGroup;
        foreach (transactionGroup, groupedTransactions) {
            transactions.append(transactionGroup);
            // NOTE: this somewhat impacts performance. But it provides useful
            // insight.
            foreach (const QStringList & transaction, transactionGroup)
                items += transaction.size();
        }

        transactionsPerEvent = ((double) transactions.size()) / batch.size();
        averageTransactionLength = 1.0 * items / transactions.size();
        emit stats(timer.elapsed(),
                   transactions.size(),
                   transactionsPerEvent,
                   averageTransactionLength,
                   lastChunkOfBatch,
                   batch.first().time,
                   batch.last().time,
                   discardedSamples);
        emit parsedBatch(transactions,
                         transactionsPerEvent,
                         batch.first().time,
                         batch.last().time,
                         quarterID,
                         lastChunkOfBatch);


        // Pause the parsing until these transactions have been processed!
        this->mutex.lock();
        this->condition.wait(&this->mutex);
        this->timer.start(); // Restart the timer.
        this->mutex.unlock();
    }


    //---------------------------------------------------------------------------
    // Protected overridable methods.

    void Parser::processParsedChunk(const QStringList & chunk, bool forceProcessing) {
        static quint32 currentQuarterID = 0;
        static QList<Config::Sample> batch;
        quint16 sampleNumber = 0;
        quint32 discardedSamples = 0;

        // Perform the mapping from strings to EpisodesLogLine concurrently.
//        QList<EpisodesLogLine> mappedChunk = QtConcurrent::blockingMapped(chunk, Parser::mapLineToEpisodesLogLine);
        QString rawSample;
        Config::Sample sample;
        foreach (rawSample, chunk) {
            // Don't waste time parsing checkpointing lines.
            if (rawSample.at(0) == '#')
                continue;

            sample = Parser::parseSample(rawSample, &this->config);

            // Discard samples without circumstances.
            if (sample.circumstances.isEmpty()) {
                discardedSamples++;
                continue;
            }

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
                    this->processBatch(batch, quarterID - 1, true, discardedSamples);
                    discardedSamples = 0;
                    batch.clear();
                }
            }

            // Ensure that the batch doesn't get too large (and thus consumes
            // too much memory): let it be processed when it has grown to
            // PROCESS_CHUNK_SIZE lines.
            if (batch.size() == PROCESS_CHUNK_SIZE) {
                this->processBatch(batch, currentQuarterID, false, discardedSamples); // The batch doesn't end here!
                discardedSamples = 0;
                batch.clear();
            }

            batch.append(sample);
        }

        if (forceProcessing && !batch.isEmpty()) {
            this->processBatch(batch, currentQuarterID, true, discardedSamples);
            discardedSamples = 0;
            batch.clear();
        }
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

        // Only a single transaction if there are no episodes!
        if (sample.episodes.isEmpty()) {
            transaction << sample.circumstances.toList();
            transactions << transaction;
        }

        return transactions;
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
