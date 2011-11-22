QT += core
QT -= gui

INCLUDEPATH += $${PWD}

DEPENDPATH += $${PWD}

include("../common/common.pri")
include("../shared/qxtjson.pri")

SOURCES += \
    $${PWD}/Item.cpp \
    $${PWD}/FPTree.cpp \
    $${PWD}/FPGrowth.cpp\
    $${PWD}/RuleMiner.cpp \
    $${PWD}/Analyst.cpp \
    $${PWD}/Constraints.cpp \
    $${PWD}/FPStream.cpp \
    $${PWD}/PatternTree.cpp \
    $${PWD}/TTWDefinition.cpp \
    $${PWD}/TiltedTimeWindow.cpp
HEADERS += \
    $${PWD}/Item.h \
    $${PWD}/FPNode.h \
    $${PWD}/FPTree.h \
    $${PWD}/FPGrowth.h \
    $${PWD}/RuleMiner.h \
    $${PWD}/Analyst.h \
    $${PWD}/Constraints.h \
    $${PWD}/FPStream.h \
    $${PWD}/PatternTree.h \
    $${PWD}/TTWDefinition.h \
    $${PWD}/TiltedTimeWindow.h

# Disable qDebug() output when in release mode.
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Add a DEBUG define when in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG
