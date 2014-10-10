include(../../common-project-config.pri)

TARGET = tst_inactivity_timer

CONFIG += \
    debug

QT += \
    core \
    testlib

ONLINE_ACCOUNTS_SERVICE_DIR = $${TOP_SRC_DIR}/online-accounts-service
COMMON_SRC_DIR = $${TOP_SRC_DIR}/online-accounts-ui

SOURCES += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/inactivity-timer.cpp \
    tst_inactivity_timer.cpp

HEADERS += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/inactivity-timer.h

INCLUDEPATH += \
    $${COMMON_SRC_DIR} \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
