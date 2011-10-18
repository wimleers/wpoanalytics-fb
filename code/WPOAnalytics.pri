DEPENDPATH += Config \
              FacebookLogParser \
              Analytics
include("Config/Config.pri")
include("FacebookLogParser/FacebookLogParser.pri")
include("Analytics/Analytics.pri")


# Enable compiler optimizations when building in release mode.
QMAKE_CXXFLAGS_RELEASE = -O3 \
    -funroll-loops \
    -fstrict-aliasing

# Cleanup.
HEADERS = $$unique(HEADERS)
DEFINES = $$unique(DEFINES)
SOURCES = $$unique(SOURCES)
INCLUDEPATH = $$unique(INCLUDEPATH)
