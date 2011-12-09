include("patternminer-base.pri")

DEFINES += INTERFACE_GRAPHICAL
DEPENDPATH += UI
include("UI/UI.pri")

SOURCES += main.cpp

TARGET = "PatternMiner GUI"
