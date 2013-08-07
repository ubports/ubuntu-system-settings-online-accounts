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
LIBS += -lonline-accounts-client

MOCK_PATH = $${TOP_SRC_DIR}/client/tests/

DEFINES += \
    MOCK_PATH=\\\"$$MOCK_PATH\\\"

chown.commands = "chmod a+x system-settings"

check.commands = "xvfb-run -a ./$${TARGET}"
check.depends = $${TARGET} chown
QMAKE_EXTRA_TARGETS += chown check
