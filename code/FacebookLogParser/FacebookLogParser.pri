QT += core
QT -= gui

INCLUDEPATH += \
    $${PWD}
DEPENDPATH += \
    $${PWD}

SOURCES += \
    $${PWD}/Parser.cpp \
    $${PWD}/typedefs.cpp \
    $${PWD}/EpisodeDurationDiscretizer.cpp \
    $${PWD}/qxtjson.cpp

HEADERS += \
    $${PWD}/Parser.h \
    $${PWD}/typedefs.h \
    $${PWD}/EpisodeDurationDiscretizer.h \
    $${PWD}/qxtjson.h

# Disable qDebug() output when in release mode.
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Add a DEBUG define when in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG
