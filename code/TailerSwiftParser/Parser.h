#ifndef TAILERSWIFTPARSER_H
#define TAILERSWIFTPARSER_H

#include "../Parser/JSONLogParser.h"

namespace TailerSwiftParser {

    class Parser : public JSONLogParser::Parser {
        Q_OBJECT

    public:
        Parser(Config::Config config);

    protected:
        virtual void processParsedChunk(const QStringList & chunk, bool forceProcessing = false);
    };

}

#endif // TAILERSWIFTPARSER_H
