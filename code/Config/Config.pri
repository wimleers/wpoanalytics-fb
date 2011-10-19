QT += core
QT -= gui

INCLUDEPATH += $${PWD}

DEPENDPATH += $${PWD}

include("../shared/qxtjson.pri")

SOURCES += \
    $${PWD}/typedefs.cpp \
    $${PWD}/Config.cpp

HEADERS += \
    $${PWD}/typedefs.h \
    $${PWD}/Config.h

# Disable qDebug() output when in release mode.
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Add a DEBUG define when in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG
