#ifndef FACEBOOKLOGPARSER_H
#define FACEBOOKLOGPARSER_H

#include <QDebug>

#include "../Parser/JSONLogParser.h"

namespace FacebookLogParser {

    class Parser : public JSONLogParser::Parser {
        Q_OBJECT

    public:
        Parser(Config::Config config);

    protected:
        virtual void processParsedChunk(const QStringList & chunk, bool forceProcessing = false);
    };

}

#endif // FACEBOOKLOGPARSER_H
