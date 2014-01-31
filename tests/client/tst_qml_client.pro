include(../../common-project-config.pri)

TARGET = tst_qml_client

CONFIG += \
    debug

QT += \
    core \
    dbus \
    qml \
    testlib

SOURCES += \
    tst_qml_client.cpp

INCLUDEPATH += \
    $${TOP_SRC_DIR}
PLUGIN_PATH = $${TOP_BUILD_DIR}/client/module
DEFINES += \
    PLUGIN_PATH=\\\"$${PLUGIN_PATH}\\\"

check.commands = "LD_LIBRARY_PATH=$${TOP_BUILD_DIR}/client/OnlineAccountsClient:${LD_LIBRARY_PATH} xvfb-run -s '-screen 0 640x480x24' -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
