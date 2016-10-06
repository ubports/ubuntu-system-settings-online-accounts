include(../../common-project-config.pri)

TARGET = tst_signonui_service

CONFIG += \
    debug \
    link_pkgconfig

QT += \
    core \
    dbus \
    network \
    testlib

PKGCONFIG += \
    accounts-qt5 \
    libapparmor \
    signon-plugins-common

DEFINES += \
    NO_REQUEST_FACTORY

ONLINE_ACCOUNTS_SERVICE_DIR = $${TOP_SRC_DIR}/online-accounts-service
COMMON_SRC_DIR = $${TOP_SRC_DIR}/online-accounts-ui

SOURCES += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/request.cpp \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/signonui-service.cpp \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/utils.cpp \
    mock/request-manager-mock.cpp \
    tst_signonui_service.cpp

HEADERS += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/request.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/request-manager.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/signonui-service.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/utils.h \
    mock/request-manager-mock.h

INCLUDEPATH += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR} \
    $${COMMON_SRC_DIR}

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
