include (../../common-project-config.pri)

TEMPLATE = lib
TARGET = OnlineAccountsClient

CONFIG += \
    qt

QT += gui

QMAKE_CXXFLAGS += \
    -fvisibility=hidden
DEFINES += BUILDING_ONLINE_ACCOUNTS_CLIENT

# Error on undefined symbols
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

public_headers += \
    global.h \
    setup.h Setup

SOURCES += \
    setup.cpp

HEADERS += \
    $${private_headers} \
    $${public_headers}

headers.files = $${public_headers}

include($${TOP_SRC_DIR}/common-installs-config.pri)

pkgconfig.files = $${TARGET}.pc
include($${TOP_SRC_DIR}/common-pkgconfig.pri)
