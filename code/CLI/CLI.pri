include("../shared/qxtcommandoptions.pri")

macx {
    CONFIG -= app_bundle
}

SOURCES += $${PWD}/CLI.cpp

HEADERS += $${PWD}/CLI.h

