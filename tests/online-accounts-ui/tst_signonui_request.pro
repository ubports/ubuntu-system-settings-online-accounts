include(online-accounts-ui.pri)

TARGET = tst_signonui_request

CONFIG += \
    link_pkgconfig

QT += \
    dbus

PKGCONFIG += \
    accounts-qt5 \
    libsignon-qt5 \
    signon-plugins-common

INCLUDEPATH += \
    $${TOP_SRC_DIR}/plugins
QMAKE_LIBDIR = $${TOP_BUILD_DIR}/plugins/OnlineAccountsPlugin
LIBS += -lonline-accounts-plugin

DEFINES += \
    NO_REQUEST_FACTORY \
    TEST_DATA_DIR=\\\"$${PWD}/data\\\"


SOURCES += \
    $${ONLINE_ACCOUNTS_UI_DIR}/debug.cpp \
    $${ONLINE_ACCOUNTS_UI_DIR}/signonui-request.cpp \
    mock/notification-mock.cpp \
    mock/request-mock.cpp \
    mock/qwindow.cpp \
    mock/ui-server-mock.cpp \
    tst_signonui_request.cpp

HEADERS += \
    $${ONLINE_ACCOUNTS_UI_DIR}/notification.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/request.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/signonui-request.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/ui-server.h \
    mock/notification-mock.h \
    mock/request-mock.h \
    mock/ui-server-mock.h \
    window-watcher.h

check.commands += "xvfb-run -s '-screen 0 640x480x24' -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
