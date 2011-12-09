include("Analytics/Tests/Tests.pri")

SOURCES += $${PWD}/Tests.cpp

CONFIG += qtestlib
macx {
  CONFIG -= app_bundle
}
TARGET = tests

include("patternminer-building.pri")
