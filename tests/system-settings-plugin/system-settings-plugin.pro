include(../../common-project-config.pri)

TARGET = tst_plugin

CONFIG += \
    debug \
    link_pkgconfig

QT += \
    core \
    qml \
    testlib

PKGCONFIG += \
    SystemSettings \
    accounts-qt5

SYSTEM_SETTINGS_PLUGIN_DIR = $${TOP_SRC_DIR}/system-settings-plugin

INCLUDEPATH += \
    $${SYSTEM_SETTINGS_PLUGIN_DIR}

SOURCES += \
    $${SYSTEM_SETTINGS_PLUGIN_DIR}/plugin.cpp \
    tst_plugin.cpp

HEADERS += \
    $${SYSTEM_SETTINGS_PLUGIN_DIR}/plugin.h

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
