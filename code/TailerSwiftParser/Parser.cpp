#include "Parser.h"

namespace TailerSwiftParser {

    Parser::Parser(Config::Config config)
        : JSONLogParser::Parser::Parser(config)
    {
    }


    //---------------------------------------------------------------------------
    // Protected overridden methods.

    /**
     * This override reacts to TailerSwift's 'swift_event' lines that are added
     * to the stream to define time windows.
     */
    void Parser::processParsedChunk(const QStringList & chunk, bool forceProcessing) {
        static quint32 windowID = 0;
        static QList<Config::Sample> batch;
        bool lastSampleWasCheckpoint = false;
        quint32 discardedSamples = 0;

        QString rawSample;
        Config::Sample sample;
        foreach (rawSample, chunk) {
            // ptail checkpoint: 15 minutes have passed!
            if (rawSample.startsWith("swift_event")) {
                // There are often multiple checkpoints after each other. Don't
                // trigger a new time window for each.
                if (!lastSampleWasCheckpoint) {
                    windowID++;
                    // Process the current batch.
                    if (!batch.isEmpty()) {
                        // The batch we just finished is the previous quarter ID!
                        this->processBatch(batch, windowID - 1, true, discardedSamples);
                        discardedSamples = 0;
                        batch.clear();
                    }
                    lastSampleWasCheckpoint = true;
                }
                else
                    lastSampleWasCheckpoint = false;
            }

            // Parse sample.
            sample = Parser::parseSample(rawSample, &this->config);

            // Discard samples without circumstances.
            if (sample.circumstances.isEmpty()) {
                discardedSamples++;
                continue;
            }

            // Ensure that the batch doesn't get too large (and thus consumes
            // too much memory): let it be processed when it has grown to
            // PROCESS_CHUNK_SIZE lines.
            if (batch.size() == PROCESS_CHUNK_SIZE) {
                this->processBatch(batch, windowID, false, discardedSamples); // The batch doesn't end here!
                discardedSamples = 0;
                batch.clear();
            }

            batch.append(sample);
        }

        // Handle incomplete batches when there's no more data.
        if (forceProcessing && !batch.isEmpty()) {
            this->processBatch(batch, windowID, true, discardedSamples);
            discardedSamples = 0;
            batch.clear();
        }
    }
}
