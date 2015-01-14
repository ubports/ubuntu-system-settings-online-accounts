include(../../common-project-config.pri)

TARGET = tst_libaccounts_service

CONFIG += \
    debug \
    link_pkgconfig

QT += \
    core \
    dbus \
    testlib \
    xml

ONLINE_ACCOUNTS_SERVICE_DIR = $${TOP_SRC_DIR}/online-accounts-service
COMMON_SRC_DIR = $${TOP_SRC_DIR}/online-accounts-ui
LIBACCOUNTS_QT_DIR = $$system(pkg-config --variable=includedir accounts-qt5)/Accounts

SOURCES += \
    $${COMMON_SRC_DIR}/debug.cpp \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/libaccounts-service.cpp \
    tst_libaccounts_service.cpp

HEADERS += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/libaccounts-service.h \
    $${ONLINE_ACCOUNTS_SERVICE_DIR}/utils.h \
    $${LIBACCOUNTS_QT_DIR}/account.h \
    $${LIBACCOUNTS_QT_DIR}/manager.h

INCLUDEPATH += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR} \
    $${COMMON_SRC_DIR} \
    $$system(pkg-config --variable=includedir accounts-qt5)

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a dbus-test-runner -t ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check

