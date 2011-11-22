QT += core
QT -= gui

INCLUDEPATH += $${PWD}

DEPENDPATH += $${PWD}

include("../common/common.pri")
include("../Config/Config.pri")
include("../shared/qxtjson.pri")

SOURCES += \
    $${PWD}/JSONLogParser.cpp

HEADERS += \
    $${PWD}/JSONLogParser.h

# Disable qDebug() output when in release mode.
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Add a DEBUG define when in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG
