#ifndef TAILERSWIFTPARSER_H
#define TAILERSWIFTPARSER_H

#include "../Parser/JSONLogParser.h"

namespace TailerSwiftParser {

    class Parser : public JSONLogParser::Parser {
        Q_OBJECT

    public:
        Parser(Config::Config config, uint secPerBatch);

    protected:
        virtual WindowMarkerMethod getWindowMarkerMethod() const;
        virtual QString getWindowMarkerLine() const;
    };

}

#endif // TAILERSWIFTPARSER_H
