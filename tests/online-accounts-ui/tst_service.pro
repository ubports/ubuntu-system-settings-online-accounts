include(../../common-project-config.pri)

TARGET = tst_service

CONFIG += \
    debug

QT += \
    core \
    dbus \
    testlib

DEFINES += \
    NO_REQUEST_FACTORY

SOURCES += \
    $${TOP_BUILD_DIR}/src/onlineaccountsui_adaptor.cpp \
    $${TOP_SRC_DIR}/src/request.cpp \
    $${TOP_SRC_DIR}/src/service.cpp \
    tst_service.cpp

HEADERS += \
    $${TOP_BUILD_DIR}/src/onlineaccountsui_adaptor.h \
    $${TOP_SRC_DIR}/src/request.h \
    $${TOP_SRC_DIR}/src/service.h

INCLUDEPATH += \
    $${TOP_SRC_DIR}/src

check.commands = "xvfb-run -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
