include("patternminer-base.pri")

DEFINES += INTERFACE_COMMANDLINE
DEPENDPATH += CLI
include("CLI/CLI.pri")

SOURCES += main.cpp

TARGET = patternminer
