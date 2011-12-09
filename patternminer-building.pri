# Cleanup: filter to unique values.
HEADERS = $$unique(HEADERS)
DEFINES = $$unique(DEFINES)
SOURCES = $$unique(SOURCES)
INCLUDEPATH = $$unique(INCLUDEPATH)

# Generic build settings.
OBJECTS_DIR = .objects
MOC_DIR = .moc
