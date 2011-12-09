QT += core
QT -= gui

INCLUDEPATH += $${PWD}

SOURCES += \
	$${PWD}/qxtglobal.cpp \
    $${PWD}/qxtcommandoptions.cpp
HEADERS += \
	$${PWD}/qxtglobal.h \
    $${PWD}/qxtcommandoptions.h

# Disable qDebug() output when in release mode.
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Add a DEBUG define when in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG
