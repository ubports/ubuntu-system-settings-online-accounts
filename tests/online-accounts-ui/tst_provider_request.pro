include(online-accounts-ui.pri)

TARGET = tst_provider_request

CONFIG += \
    link_pkgconfig

QT += \
    gui \
    quick

PKGCONFIG += \
    accounts-qt5

INCLUDEPATH += \
    $${TOP_SRC_DIR}/plugins
QMAKE_LIBDIR = $${TOP_BUILD_DIR}/plugins/OnlineAccountsPlugin
LIBS += -lonline-accounts-plugin

DEFINES += \
    NO_REQUEST_FACTORY \
    OAU_PLUGIN_DIR=\\\"/tmp\\\" \
    PLUGIN_PRIVATE_MODULE_DIR=\\\"/tmp\\\" \
    TEST_DATA_DIR=\\\"$${PWD}/data\\\"


SOURCES += \
    $${ONLINE_ACCOUNTS_UI_DIR}/access-model.cpp \
    $${ONLINE_ACCOUNTS_UI_DIR}/debug.cpp \
    $${ONLINE_ACCOUNTS_UI_DIR}/i18n.cpp \
    $${ONLINE_ACCOUNTS_UI_DIR}/provider-request.cpp \
    mock/application-manager-mock.cpp \
    mock/request-mock.cpp \
    mock/ui-server-mock.cpp \
    tst_provider_request.cpp

HEADERS += \
    $${ONLINE_ACCOUNTS_UI_DIR}/access-model.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/i18n.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/provider-request.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/request.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/ui-server.h \
    mock/application-manager-mock.h \
    mock/request-mock.h \
    mock/ui-server-mock.h

RESOURCES += \
    tst_provider_request.qrc

check.commands += "xvfb-run -s '-screen 0 640x480x24' -a ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
