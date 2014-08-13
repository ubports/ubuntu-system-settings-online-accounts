include(../../common-project-config.pri)

TARGET = tst_service

CONFIG += \
    debug

QT += \
    core \
    dbus \
    testlib

DEFINES += \
    NO_REQUEST_FACTORY

ONLINE_ACCOUNTS_SERVICE_DIR = $${TOP_SRC_DIR}/online-accounts-service
COMMON_SRC_DIR = $${TOP_SRC_DIR}/online-accounts-ui

SOURCES += \
    $${TOP_BUILD_DIR}/online-accounts-service/onlineaccountsui_adaptor.cpp \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/request.cpp \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/request-manager.cpp \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/service.cpp \
    tst_service.cpp

HEADERS += \
    $${TOP_BUILD_DIR}/online-accounts-service/onlineaccountsui_adaptor.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/request.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/request-manager.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/service.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/ui-proxy.h \

INCLUDEPATH += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR} \
    $${COMMON_SRC_DIR}

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
