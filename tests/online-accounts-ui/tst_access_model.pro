include(../../common-project-config.pri)

TARGET = tst_access_model

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
    $${TOP_SRC_DIR}/src/access-model.cpp \
    $${TOP_SRC_DIR}/src/account-manager.cpp \
    $${TOP_SRC_DIR}/src/debug.cpp \
    tst_access_model.cpp

HEADERS += \
    $${TOP_SRC_DIR}/src/access-model.h \
    $${TOP_SRC_DIR}/src/account-manager.h \
    $${TOP_SRC_DIR}/src/debug.h

INCLUDEPATH += \
    $${TOP_SRC_DIR}/src

check.commands = "xvfb-run -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
