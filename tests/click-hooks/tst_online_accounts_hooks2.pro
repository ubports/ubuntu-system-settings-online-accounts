include(../../common-project-config.pri)

TARGET = tst_online_accounts_hooks2

CONFIG += \
    c++11 \
    debug \
    link_pkgconfig

QT += \
    core \
    dbus \
    testlib \
    xml

PKGCONFIG += \
    accounts-qt5 \
    libqtdbusmock-1 \
    libqtdbustest-1 \
    libsignon-qt5

DEFINES += \
    DEBUG_ENABLED \
    HOOK_PROCESS=\\\"../../click-hooks/online-accounts-hooks2\\\" \
    SIGNOND_MOCK_TEMPLATE=\\\"$${PWD}/signond.py\\\"

SOURCES += \
    tst_online_accounts_hooks2.cpp

check.commands = "xvfb-run -s '-screen 0 640x480x24' -a ./$${TARGET}"
check.depends = $${TARGET}
QMAKE_EXTRA_TARGETS += check
