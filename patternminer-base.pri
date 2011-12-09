include("Config/Config.pri")
include("TailerSwiftParser/TailerSwiftParser.pri")
include("Analytics/Analytics.pri")

# Enable compiler optimizations when building in release mode.
QMAKE_CXXFLAGS_RELEASE = -O3 \
    -funroll-loops \
    -fstrict-aliasing

include("patternminer-building.pri")
