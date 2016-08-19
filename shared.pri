######################################################################
# Communi
######################################################################

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD
DEFINES += BUILD_SHARED

HEADERS += $$PWD/ignoremanager.h
HEADERS += $$PWD/messagehandler.h
HEADERS += $$PWD/networksession.h
HEADERS += $$PWD/sharedglobal.h
HEADERS += $$PWD/sharedtimer.h
HEADERS += $$PWD/zncmanager.h

SOURCES += $$PWD/ignoremanager.cpp
SOURCES += $$PWD/messagehandler.cpp
SOURCES += $$PWD/networksession.cpp
SOURCES += $$PWD/sharedtimer.cpp
SOURCES += $$PWD/zncmanager.cpp
