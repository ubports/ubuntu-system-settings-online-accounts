include(../../common-project-config.pri)

TARGET = tst_notification

CONFIG += \
    debug \
    link_pkgconfig \
    no_keywords

QT += \
    core \
    testlib

PKGCONFIG += \
    libnotify

SOURCES += \
    $${TOP_SRC_DIR}/src/notification.cpp \
    tst_notification.cpp

HEADERS += \
    $${TOP_SRC_DIR}/src/notification.h

INCLUDEPATH += \
    $${TOP_SRC_DIR}/src

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
