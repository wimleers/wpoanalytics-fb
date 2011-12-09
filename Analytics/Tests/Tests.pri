include (../Analytics.pri)

HEADERS += $${PWD}/Tests.h \
           $${PWD}/TestFPTree.h \
           $${PWD}/TestConstraints.h \
           $${PWD}/TestFPGrowth.h \
           $${PWD}/TestRuleMiner.h \
           $${PWD}/TestTTWDefinition.h \
           $${PWD}/TestTiltedTimeWindow.h \
           $${PWD}/TestPatternTree.h \
           $${PWD}/TestFPStream.h
SOURCES += $${PWD}/TestConstraints.cpp \
           $${PWD}/TestFPTree.cpp \
           $${PWD}/TestFPGrowth.cpp \
           $${PWD}/TestRuleMiner.cpp \
           $${PWD}/TestTTWDefinition.cpp \
           $${PWD}/TestTiltedTimeWindow.cpp \
           $${PWD}/TestPatternTree.cpp \
           $${PWD}/TestFPStream.cpp
