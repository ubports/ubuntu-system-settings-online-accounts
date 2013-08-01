include(../../common-project-config.pri)

TARGET = tst_client

CONFIG += \
    debug

QT += \
    core \
    testlib

SOURCES += \
    tst_client.cpp

INCLUDEPATH += \
    $$TOP_SRC_DIR/client
QMAKE_LIBDIR = $${TOP_BUILD_DIR}/client/OnlineAccountsClient
QMAKE_RPATHDIR = $${QMAKE_LIBDIR}
LIBS += -lOnlineAccountsClient

MOCK_PATH = $${TOP_SRC_DIR}/client/tests/

DEFINES += \
    MOCK_PATH=\\\"$$MOCK_PATH\\\"

check.commands = "xvfb-run -a ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
