#include "Parser.h"

namespace TailerSwiftParser {

    Parser::Parser(Config::Config config, uint secPerBatch)
        : JSONLogParser::Parser::Parser(config, secPerBatch)
    {
    }


    //---------------------------------------------------------------------------
    // Protected overridden methods.

    WindowMarkerMethod Parser::getWindowMarkerMethod() const {
        return WindowMarkerMethodMarkerLine;
    }

    /**
     * This override reacts to TailerSwift's 'swift_event' lines that are added
     * to the stream to define time windows.
     */
    QString Parser::getWindowMarkerLine() const {
        return QString("swift_event");
    }
}
