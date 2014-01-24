include(../../common-project-config.pri)

TARGET = tst_inactivity_timer

CONFIG += \
    debug

QT += \
    core \
    testlib

SOURCES += \
    $${TOP_SRC_DIR}/src/inactivity-timer.cpp \
    tst_inactivity_timer.cpp

HEADERS += \
    $${TOP_SRC_DIR}/src/inactivity-timer.h

INCLUDEPATH += \
    $${TOP_SRC_DIR}/src

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
