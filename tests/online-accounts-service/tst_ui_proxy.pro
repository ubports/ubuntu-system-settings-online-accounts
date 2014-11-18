include(../../common-project-config.pri)

TARGET = tst_ui_proxy

CONFIG += \
    debug \
    link_pkgconfig

QT += \
    core \
    dbus \
    network \
    testlib

DEFINES += \
    INSTALL_BIN_DIR=\\\"$${INSTALL_PREFIX}/bin\\\"

PKGCONFIG += \
    accounts-qt5 \
    signon-plugins-common \
    ubuntu-app-launch-2

ONLINE_ACCOUNTS_SERVICE_DIR = $${TOP_SRC_DIR}/online-accounts-service
COMMON_SRC_DIR = $${TOP_SRC_DIR}/online-accounts-ui

SOURCES += \
    $${COMMON_SRC_DIR}/ipc.cpp \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/mir-helper-stub.cpp \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/ui-proxy.cpp \
    mock/request-mock.cpp \
    tst_ui_proxy.cpp

HEADERS += \
    $${COMMON_SRC_DIR}/ipc.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/mir-helper.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/request.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/ui-proxy.h \
    mock/request-mock.h

INCLUDEPATH += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR} \
    $${COMMON_SRC_DIR}

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
