QT += core
QT -= gui

INCLUDEPATH += $${PWD}

DEPENDPATH += $${PWD}

include("../Config/config.pri")
include("../shared/qxtjson.pri")

SOURCES += \
    $${PWD}/Parser.cpp

HEADERS += \
    $${PWD}/Parser.h

# Disable qDebug() output when in release mode.
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Add a DEBUG define when in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG
