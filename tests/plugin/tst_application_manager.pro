include(plugin.pri)

TARGET = tst_application_manager

CONFIG += \
    link_pkgconfig

QT += \
    dbus \
    qml
QT -= gui

PKGCONFIG += \
    accounts-qt5

DEFINES += \
    TEST_DATA_DIR=\\\"$${PWD}/data\\\"

SOURCES += \
    $${ONLINE_ACCOUNTS_PLUGIN_DIR}/application-manager.cpp \
    $${ONLINE_ACCOUNTS_PLUGIN_DIR}/account-manager.cpp \
    tst_application_manager.cpp

HEADERS += \
    $${ONLINE_ACCOUNTS_PLUGIN_DIR}/application-manager.h \
    $${ONLINE_ACCOUNTS_PLUGIN_DIR}/account-manager.h

check.commands = "xvfb-run -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
