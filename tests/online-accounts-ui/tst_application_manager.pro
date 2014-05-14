include(../../common-project-config.pri)

TARGET = tst_application_manager

CONFIG += \
    debug \
    link_pkgconfig

QT += \
    core \
    dbus \
    qml \
    testlib

PKGCONFIG += \
    accounts-qt5

DEFINES += \
    DEBUG_ENABLED \
    TEST_DATA_DIR=\\\"$${PWD}/data\\\"

SOURCES += \
    $${TOP_SRC_DIR}/src/application-manager.cpp \
    $${TOP_SRC_DIR}/src/account-manager.cpp \
    $${TOP_SRC_DIR}/src/debug.cpp \
    tst_application_manager.cpp

HEADERS += \
    $${TOP_SRC_DIR}/src/application-manager.h \
    $${TOP_SRC_DIR}/src/account-manager.h \
    $${TOP_SRC_DIR}/src/debug.h

INCLUDEPATH += \
    $${TOP_SRC_DIR}/src

check.commands = "xvfb-run -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
