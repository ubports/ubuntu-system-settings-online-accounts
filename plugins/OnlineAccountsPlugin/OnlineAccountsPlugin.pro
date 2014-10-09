include (../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = online-accounts-plugin

CONFIG += \
    link_pkgconfig \
    qt

QT += \
    gui \
    network

PKGCONFIG += \
    accounts-qt5 \
    signon-plugins-common

QMAKE_CXXFLAGS += \
    -fvisibility=hidden
DEFINES += BUILDING_ONLINE_ACCOUNTS_PLUGIN

# Error on undefined symbols
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

private_headers += \
    account-manager.h \
    application-manager.h \
    request-handler.h

public_headers +=

INCLUDEPATH += \
    $${TOP_SRC_DIR}

SOURCES += \
    account-manager.cpp \
    application-manager.cpp \
    request-handler.cpp

HEADERS += \
    $${private_headers} \
    $${public_headers}

headers.files = $${public_headers}

include($${TOP_SRC_DIR}/common-installs-config.pri)

pkgconfig.files = OnlineAccountsPlugin.pc
include($${TOP_SRC_DIR}/common-pkgconfig.pri)
