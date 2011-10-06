DEPENDPATH += FacebookLogParser \
              Analytics \
              UI

include("FacebookLogParser/FacebookLogParser.pri")
include("Analytics/Analytics.pri")
include("UI/UI.pri")


# Enable compiler optimizations when building in release mode.
QMAKE_CXXFLAGS_RELEASE = -O3 \
    -funroll-loops \
    -fstrict-aliasing

SOURCES += main.cpp
