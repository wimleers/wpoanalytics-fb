DEPENDPATH += ..
include (../Analytics.pri)

CONFIG += qtestlib
macx {
  CONFIG -= app_bundle
}
TARGET = Tests


HEADERS += TestFPTree.h \
           TestConstraints.h \
           TestFPGrowth.h \
           TestRuleMiner.h \
           TestTTWDefinition.h \
           TestTiltedTimeWindow.h \
           TestPatternTree.h \
           TestFPStream.h
SOURCES += Tests.cpp \
           TestConstraints.cpp \
           TestFPTree.cpp \
           TestFPGrowth.cpp \
           TestRuleMiner.cpp \
           TestTTWDefinition.cpp \
           TestTiltedTimeWindow.cpp \
           TestPatternTree.cpp \
           TestFPStream.cpp
