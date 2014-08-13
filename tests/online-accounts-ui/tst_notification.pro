include(online-accounts-ui.pri)

TARGET = tst_notification

CONFIG += \
    link_pkgconfig \
    no_keywords

PKGCONFIG += \
    libnotify

SOURCES += \
    $${COMMON_SRC_DIR}/notification.cpp \
    tst_notification.cpp

HEADERS += \
    $${COMMON_SRC_DIR}/notification.h

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
