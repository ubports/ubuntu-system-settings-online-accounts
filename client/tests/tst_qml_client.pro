include(../../common-project-config.pri)

TARGET = tst_qml_client

CONFIG += \
    debug

QT += \
    core \
    qml \
    testlib

SOURCES += \
    tst_qml_client.cpp

MOCK_PATH = $${TOP_SRC_DIR}/client/tests/

DEFINES += \
    MOCK_PATH=\\\"$$MOCK_PATH\\\"

check.commands = "LD_LIBRARY_PATH=$${TOP_BUILD_DIR}/client/OnlineAccountsClient:${LD_LIBRARY_PATH} xvfb-run -a ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
