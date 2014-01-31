include (../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = online-accounts-client

CONFIG += \
    qt

QT += \
    dbus \
    gui

QMAKE_CXXFLAGS += \
    -fvisibility=hidden
DEFINES += BUILDING_ONLINE_ACCOUNTS_CLIENT

# Error on undefined symbols
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

public_headers += \
    global.h \
    setup.h Setup

INCLUDEPATH += \
    $${TOP_SRC_DIR}

ONLINE_ACCOUNTS_UI_SRC = $${TOP_SRC_DIR}/src

DBUS_INTERFACES += \
    $${ONLINE_ACCOUNTS_UI_SRC}/com.ubuntu.OnlineAccountsUi.xml

SOURCES += \
    setup.cpp

HEADERS += \
    $${private_headers} \
    $${public_headers}

headers.files = $${public_headers}

include($${TOP_SRC_DIR}/common-installs-config.pri)

pkgconfig.files = OnlineAccountsClient.pc
include($${TOP_SRC_DIR}/common-pkgconfig.pri)
