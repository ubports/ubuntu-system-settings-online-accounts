include(online-accounts-ui.pri)

TARGET = tst_access_model

CONFIG += \
    link_pkgconfig

QT += \
    dbus \
    qml

PKGCONFIG += \
    accounts-qt5

INCLUDEPATH += \
    $${TOP_SRC_DIR}/plugins
QMAKE_LIBDIR = $${TOP_BUILD_DIR}/plugins/OnlineAccountsPlugin
LIBS += -lonline-accounts-plugin

DEFINES += \
    TEST_DATA_DIR=\\\"$${PWD}/data\\\"

SOURCES += \
    $${ONLINE_ACCOUNTS_UI_DIR}/access-model.cpp \
    $${ONLINE_ACCOUNTS_UI_DIR}/debug.cpp \
    tst_access_model.cpp

HEADERS += \
    $${ONLINE_ACCOUNTS_UI_DIR}/access-model.h \
    $${ONLINE_ACCOUNTS_UI_DIR}/debug.h

check.commands += "xvfb-run -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
