QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5

QT += xml widgets

TARGET = cg3ide
TEMPLATE = app

CODECFORSRC = UTF-8
CODECFORTR = UTF-8

RC_FILE = cg3ide.rc
ICON = cg3ide.icns

SOURCES += src/main.cpp \
    src/GrammarEditor.cpp \
    src/GotoLine.cpp \
    src/StreamHighlighter.cpp \
    src/GrammarHighlighter.cpp \
    src/OptionsDialog.cpp

HEADERS  += \
    src/GrammarEditor.hpp \
    src/GotoLine.hpp \
    src/inlines.hpp \
    src/types.hpp \
    src/StreamHighlighter.hpp \
    src/GrammarHighlighter.hpp \
    src/GrammarState.hpp \
    src/OptionsDialog.hpp \
    src/version.hpp

FORMS    += \
    src/GrammarEditor.ui \
    src/GotoLine.ui \
    src/OptionsDialog.ui

CONFIG += warn_on
