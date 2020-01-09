include(common.pri)

TARGET = cg3processor
TEMPLATE = app

CODECFORSRC = UTF-8
CODECFORTR = UTF-8

SOURCES += \
    src/Processor.cpp

HEADERS  += \
    src/inlines.hpp \
    src/Processor.hpp

FORMS    += \
    src/Processor.ui

CONFIG += warn_on
