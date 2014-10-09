include(online-accounts-ui.pri)

TARGET = tst_browser_request

CONFIG += \
    link_pkgconfig

QT += \
    dbus \
    gui \
    network \
    quick

PKGCONFIG += \
    libsignon-qt5 \
    signon-plugins-common

INCLUDEPATH += \
    $${TOP_SRC_DIR}/plugins
QMAKE_LIBDIR = $${TOP_BUILD_DIR}/plugins/OnlineAccountsPlugin
LIBS += -lonline-accounts-plugin

DEFINES += \
    NO_REQUEST_FACTORY \
    PLUGIN_PRIVATE_MODULE_DIR=\\\"/tmp\\\" \
    SIGNONUI_FAIL_TIMEOUT=100 \
    SIGNONUI_I18N_DOMAIN=\\\"translations\\\" \
    TEST_DATA_DIR=\\\"$${PWD}/data\\\"


SOURCES += \
    $${ONLINE_ACCOUNTS_UI_DIR}/browser-request.cpp \
    $${ONLINE_ACCOUNTS_UI_DIR}/debug.cpp \
    $${ONLINE_ACCOUNTS_UI_DIR}/dialog.cpp \
    $${ONLINE_ACCOUNTS_UI_DIR}/i18n.cpp \
    mock/request-mock.cpp \
    mock/signonui-request-mock.cpp \
    mock/ui-server-mock.cpp \
    tst_browser_request.cpp

HEADERS += \
    $${ONLINE_ACCOUNTS_UI_DIR}/browser-request.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/dialog.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/i18n.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/request.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/signonui-request.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/ui-server.h \
    mock/request-mock.h \
    mock/signonui-request-mock.h \
    mock/ui-server-mock.h

check.commands += "xvfb-run -s '-screen 0 640x480x24' -a ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
