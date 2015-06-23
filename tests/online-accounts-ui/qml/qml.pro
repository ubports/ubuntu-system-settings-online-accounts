include(../../../common-project-config.pri)

TARGET = tst_online_accounts_qml

CONFIG += \
    qmltestcase \
    warn_on

DEFINES += \
    SOURCE_MODULE_PATH=\\\"$${PWD}\\\" \
    TEST_DATA_DIR=\\\"$${PWD}/../data\\\"

SOURCES += \
    tst_online_accounts_qml.cpp

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a dbus-test-runner -t "
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
