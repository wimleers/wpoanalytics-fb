#include "Parser.h"

namespace FacebookLogParser {
    EpisodeNameIDHash Parser::episodeNameIDHash;
    EpisodeIDNameHash Parser::episodeIDNameHash;
    bool Parser::parserHelpersInitialized = false;

    EpisodeDurationDiscretizer Parser::episodeDiscretizer;

    QMutex Parser::parserHelpersInitMutex;
    QMutex Parser::episodeHashMutex;

    Parser::Parser() {
        Parser::parserHelpersInitMutex.lock();
        if (!Parser::parserHelpersInitialized)
            qFatal("Call Parser::initParserHelper()  before creating Parser instances.");
        Parser::parserHelpersInitMutex.unlock();
    }

    void Parser::initParserHelpers(const QString & episodeDiscretizerCSV) {
        Parser::parserHelpersInitMutex.lock();
        if (!Parser::parserHelpersInitialized) {
            // No significant permanent memory consumption.
            Parser::episodeDiscretizer.parseCsvFile(episodeDiscretizerCSV);

            Parser::parserHelpersInitialized = true;
        }
        Parser::parserHelpersInitMutex.unlock();
    }

    /**
     * Clear parser helpers' caches.
     *
     * Call this function whenever the Parser will not be used for long
     * periods of time.
     */
    void Parser::clearParserHelperCaches() {
        Parser::parserHelpersInitMutex.lock();
        if (Parser::parserHelpersInitialized) {
            Parser::parserHelpersInitialized = false;
        }
        Parser::parserHelpersInitMutex.unlock();
    }


    //---------------------------------------------------------------------------
    // Protected methods.


    /**
     * Parse the given Episodes log file.
     *
     * Emits a signal for every chunk (with CHUNK_SIZE lines). This signal can
     * then be used to process that chunk. This allows multiple chunks to be
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
            bool firstLine = true;
            while (!in.atEnd()) {
                // Skip the first line (which contains header information).
                if (firstLine) {
                    firstLine = false;
                    in.readLine();
                    continue;
                }

                chunk.append(in.readLine());
                numLines++;
                if (chunk.size() == CHUNK_SIZE) {
                    this->processParsedChunk(chunk);
                    chunk.clear();
                }
            }

            // Check if we have another chunk (with size < CHUNK_SIZE).
            if (chunk.size() > 0) {
                this->processParsedChunk(chunk);
            }
            qDebug() << "parsing ready!";
            // Notify the UI.
            emit parsing(false);
        }

        Parser::clearParserHelperCaches();
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
     * @return
     *   The corresponding episode ID.
     *
     * Modifies a class variable upon some calls (i.e. when a new key must be
     * inserted in the QHash), hence we need to use a mutex to ensure thread
     * safety.
     */
    EpisodeID Parser::mapEpisodeNameToID(EpisodeName name) {
        if (!Parser::episodeNameIDHash.contains(name)) {
            Parser::episodeHashMutex.lock();

            EpisodeID id = Parser::episodeNameIDHash.size();
            Parser::episodeNameIDHash.insert(name, id);
            Parser::episodeIDNameHash.insert(id, name);

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
    Sample Parser::parseSample(const QString & rawSample) {
        QStringList list, episodesRaw, episodeParts;
        Sample sample;
        Episode episode;

        // rawSample is of the format
        // ("1317792137", "10000", ""Tue", " Oct 04 2011", " 10:22:17 PM"",
        //  "39", "prod", "997", "1065", "1777", "4374", "56522", "CL",
        //  "328660", "3098", "7835", "yes", "14", "33016", "9022", "465",
        //  "WebKit", "535.1", "453330", "Chrome", ""Chrome 14"", "ash2c03",
        //  "prod", "yes", "14", "WinXP", "yes", "66.220.153.23", "NORMAL",
        //  "/widgets/like.php", "no", "9999", "328660", "-", "CL", "yes",
        // "es_LA", "broadband", "535.1", "WebKit", "3127544975",
        // "5659874133042365850", "10.144.130.121", "812423451",
        // "10.144.130.121", "812423451")
        list = rawSample.split(',');

        // DBG output while I was being driven insane by Scuba's .csv output!
//        qDebug() << list;
//        for (int i = 0; i < list.size(); i++)
//            qDebug() << i << "->" <<  list.at(i);

        // 0: timestamp
        sample.time = list[0].toInt();

        // 1: no idea.
        // drop

        // 2,3,4: human-readable date/time (this is actually one column, but my
        // simple & crappy parser (split on comma) parses this wrong
        // drop

        // 5: server time
        episode.id = Parser::mapEpisodeNameToID("ServerTime");
        episode.duration = list[5].toInt();
#ifdef DEBUG
        episode.IDNameHash = &Parser::episodeIDNameHash;
#endif
        sample.episodes.append(episode);

        // 6: early flush time (UNRELIABLE!)
        episode.id = Parser::mapEpisodeNameToID("EarlyFlushTime");
        episode.duration = list[6].toInt(); // Bug in current CSV's: when it is 0, some string is in here. toInt() actually makes this a 0 again.
#ifdef DEBUG
        episode.IDNameHash = &Parser::episodeIDNameHash;
#endif
        if (episode.duration > 0)
            sample.episodes.append(episode);

        // 7: network time (UNRELIABLE!)
        episode.id = Parser::mapEpisodeNameToID("NetworkTime");
        episode.duration = list[7].toInt(); // Bug in current CSV's: when it is 0, some string is in here. toInt() actually makes this a 0 again.
#ifdef DEBUG
        episode.IDNameHash = &Parser::episodeIDNameHash;
#endif
        if (episode.duration > 0)
            sample.episodes.append(episode);

        // 8: TTI (UNRELIABLE!)
        episode.id = Parser::mapEpisodeNameToID("TTI");
        episode.duration = list[8].toInt(); // Bug in current CSV's: when it is 0, some string is in here. toInt() actually makes this a 0 again.
#ifdef DEBUG
        episode.IDNameHash = &Parser::episodeIDNameHash;
#endif
        if (episode.duration > 0)
            sample.episodes.append(episode);

        // 9: e2e time (UNRELIABLE!)
        episode.id = Parser::mapEpisodeNameToID("e2eTime");
        episode.duration = list[9].toInt(); // Bug in current CSV's: when it is 0, some string is in here. toInt() actually makes this a 0 again.
#ifdef DEBUG
        episode.IDNameHash = &Parser::episodeIDNameHash;
#endif
        if (episode.duration > 0)
            sample.episodes.append(episode);

        // 10: JS orig bytes (UNRELIABLE!)
        // drop

        // 11: JS file bytes (UNRELIABLE!)
        // drop

        // 12: JS pkg bytes (UNRELIABLE!)
        // drop

        // 13: JS packages (UNRELIABLE!)
        // drop

        // 14: CSS orig bytes (UNRELIABLE!)
        // drop

        // 15: CSS file bytes (UNRELIABLE!)
        // drop

        // 16: CSS pkg bytes (UNRELIABLE!)
        // drop

        // 17: CSS packages (UNRELIABLE!)
        // drop

        // 18: HTML bytes (UNRELIABLE!)
        // drop

        // 19: HTML bytes gzipped (UNRELIABLE!)
        // drop

        // 20: cookie bytes (UNRELIABLE!)
        // drop

        // 21: zoom level (UNRELIABLE!)
        // drop

        // 22: zipcode (UNRELIABLE!)
        // drop

        // 23: svn revision
//        sample.circumstances.append("svnRev:" + list[23]);

        // 24: browser
        sample.circumstances.append("browser:" + list[24].remove('"'));

        // 25: browser major RELIABLE
        // drop

        // 26: colo
            //sample.circumstances.append("colo:" + list[26]);

        // 27: beta
            //sample.circumstances.append("beta:" + list[27]);

        // 28: valid -> no idea

        // 29: major
        sample.circumstances.append("browser:" + list[24].remove('"') + ":" + list[29]);

        // 30: platform
        sample.circumstances.append("platform:" + list[30].remove('"'));

        // 31: valid record -> no idea

        // 32: VIP
//        sample.circumstances.append("vip:" + list[32]);

        // 33: page type
            //sample.circumstances.append("pageType:" + list[33]);

        // 34: page
            sample.circumstances.append("page:" + list[34]);

        // 35: is HTTPS
            //sample.circumstances.append("isHTTPS:" + list[35]);

        // 36: cohort
        // drop

        // 37: global city
// This is too detailed, it doesn't allow us to find aggregate problems.
//        sample.circumstances.append("globalCity:" + list[37]);

        // 38: site category (UNRELIABLE!)
        // instead of either a string such as "m_basic" or "m_faceweb_ipad" or
        // (in most cases) just "-", there sometimes is a number here (the
        // number of JS packages); when this happens, map it to "-"
// meaningless most of the time, since it'll be "-" most of the time
//        sample.circumstances.append("siteCategory:" + ((list[38].toInt() != 0) ? "-" : list[38]));

        // 39: country
        sample.circumstances.append("country:" + list[39]);

        // 40: loggedIn
// this is virtually always the case
//        sample.circumstances.append("loggedIn:" + list[40]);

        // 41: locale
// implied by country, negligible performance impact
//        sample.circumstances.append("locale:" + list[41]);

        // 42: connection speed
        sample.circumstances.append("connectionSpeed:" + list[42]);

        // 43: rendering engine version (also include the rendering engine)
// implied by browser
//        sample.circumstances.append("renderingEngine:" + list[44] + ":" + list[43]);

        // 44: rendering engine
// implied by browser
// sample.circumstances.append("renderingEngine:" + list[21]);

        // 45: ip
        // drop

        // 46:id
        // drop

        // 47: server
        // drop

        // 48: uid
        // drop

        // 49: server AGAIN
        // 50: uid AGAIN



        // DBG output
//        qDebug() << sample;

//        static int n = 0;
//        n++;
//        if (n == 11) {
//            exit(1);
//        }

        return sample;
    }

    QList<QStringList> Parser::mapSampleToTransactions(const Sample & sample) {
        QList<QStringList> transactions;

        Episode episode;
        EpisodeName episodeName;
        QStringList transaction;
        foreach (episode, sample.episodes) {
            episodeName = Parser::episodeIDNameHash[episode.id];
            transaction << QString("episode:") + episodeName
                        << QString("duration:") + Parser::episodeDiscretizer.mapToSpeed(episodeName, episode.duration)
                        // Append the circumstances.
                        << sample.circumstances;
            transactions << transaction;
            transaction.clear();
        }

        return transactions;
    }


    //---------------------------------------------------------------------------
    // Protected slots.

    void Parser::processBatch(const QList<Sample> batch) {
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
        Sample sample;
        foreach (sample, batch) {
            groupedTransactions << Parser::mapSampleToTransactions(sample);
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
        emit parsedBatch(transactions, transactionsPerEvent, batch.first().time, batch.last().time);


        // Pause the parsing until these transactions have been processed!
        this->mutex.lock();
        this->condition.wait(&this->mutex);
        this->timer.start(); // Restart the timer.
        this->mutex.unlock();
    }


    //---------------------------------------------------------------------------
    // Protected methods.

    void Parser::processParsedChunk(const QStringList & chunk) {
        qDebug() << "processing parsed chunk";
        static unsigned int quarterID = 0;
        static QList<Sample> batch;


        // Perform the mapping from strings to EpisodesLogLine concurrently.
//        QList<EpisodesLogLine> mappedChunk = QtConcurrent::blockingMapped(chunk, Parser::mapLineToEpisodesLogLine);
        QString rawSample;
        Sample sample;
        foreach (rawSample, chunk) {
            sample = Parser::parseSample(rawSample);

            // Create a batch for each quarter (900 seconds) and process it.
            // TRICKY: this also ensures that quarters that have already been
            // processed are not processed again (if it is attempted to parse
            // the same file multiple times), plus it forces the user to parse
            // older files first.
            // FIXME: if file A does not end with a full quarter, i.e. a file B
            // contains the remaining episodes of a quarter, these episodes are
            // ignored. Considering that this only affects quarters, this bug
            // is ignored for now. It doesn't significantly influence the
            // results of the data set used for testing this master thesis.
            if (sample.time / 2 > quarterID) {
                quarterID = sample.time / 2;
                if (!batch.isEmpty()) {
                    this->processBatch(batch);
                    batch.clear();
                }
            }

            batch.append(sample);
        }

        if (!batch.isEmpty()) {
            this->processBatch(batch);
            batch.clear();
        }

        qDebug() << "processing done!";
    }
}
