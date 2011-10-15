QT += gui

INCLUDEPATH += $${PWD}

SOURCES += \
    $${PWD}/MainWindow.cpp \
    $${PWD}/ConceptHierarchyModel.cpp \
    $${PWD}/ConceptHierarchyCompleter.cpp \
    $${PWD}/CausesTableFilterProxyModel.cpp \
    $${PWD}/SettingsDialog.cpp

HEADERS += \
    $${PWD}/MainWindow.h \
    $${PWD}/ConceptHierarchyModel.h \
    $${PWD}/ConceptHierarchyCompleter.h \
    $${PWD}/CausesTableFilterProxyModel.h \
    $${PWD}/SettingsDialog.h

# Disable qDebug() output when in release mode.
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# Add a DEBUG define when in debug mode.
CONFIG(debug, debug|release):DEFINES += DEBUG
