include(../../common-project-config.pri)

TARGET = tst_signonui_request

CONFIG += \
    debug \
    link_pkgconfig

QT += \
    core \
    dbus \
    testlib

PKGCONFIG += \
    accounts-qt5 \
    libsignon-qt5 \
    signon-plugins-common

DEFINES += \
    DEBUG_ENABLED \
    NO_REQUEST_FACTORY \
    TEST_DATA_DIR=\\\"$${PWD}/data\\\"

SRCDIR = $${TOP_SRC_DIR}/src

SOURCES += \
    $${SRCDIR}/account-manager.cpp \
    $${SRCDIR}/application-manager.cpp \
    $${SRCDIR}/debug.cpp \
    $${SRCDIR}/signonui-request.cpp \
    mock/notification-mock.cpp \
    mock/request-mock.cpp \
    mock/qwindow.cpp \
    tst_signonui_request.cpp

HEADERS += \
    $${SRCDIR}/account-manager.h \
    $${SRCDIR}/application-manager.h \
    $${SRCDIR}/notification.h \
    $${SRCDIR}/request.h \
    $${SRCDIR}/signonui-request.h \
    mock/notification-mock.h \
    mock/request-mock.h \
    window-watcher.h

INCLUDEPATH += \
    $${SRCDIR}

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
