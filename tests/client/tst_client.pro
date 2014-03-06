include(../../common-project-config.pri)

TARGET = tst_client

CONFIG += \
    debug

QT += \
    core \
    dbus \
    testlib

SOURCES += \
    tst_client.cpp

INCLUDEPATH += \
    $${TOP_SRC_DIR}/client \
    $${TOP_SRC_DIR}

QMAKE_LIBDIR = $${TOP_BUILD_DIR}/client/OnlineAccountsClient
QMAKE_RPATHDIR = $${QMAKE_LIBDIR}
LIBS += -lonline-accounts-client

check.commands = "xvfb-run -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
