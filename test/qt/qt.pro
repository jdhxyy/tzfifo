TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.c \
    ../../tzfifo.c \
    ../../clib/tzmalloc/bget.c \
    ../../clib/tzmalloc/tzmalloc.c

INCLUDEPATH += ../../
INCLUDEPATH += ../../clib/tzmalloc

HEADERS += \
    ../../tzfifo.h \
    ../../clib/tzmalloc/bget.h \
    ../../clib/tzmalloc/tzmalloc.h
