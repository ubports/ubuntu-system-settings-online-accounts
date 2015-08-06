include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = OnlineAccountsClient

API_URI = "Ubuntu.OnlineAccounts.Client"
API_VER = 0.2

DESTDIR = $$replace(API_URI, \\., /)
PLUGIN_INSTALL_BASE = $$[QT_INSTALL_QML]/$${DESTDIR}

CONFIG += \
    plugin \
    qt

QT += qml

# Error on undefined symbols
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES = \
    plugin.cpp

HEADERS += \
    plugin.h

INCLUDEPATH += \
    $$TOP_SRC_DIR/client
QMAKE_LIBDIR = $${TOP_BUILD_DIR}/client/OnlineAccountsClient
LIBS += -lonline-accounts-client

QMLDIR_FILES += qmldir
QMAKE_SUBSTITUTES += qmldir.in
OTHER_FILES += qmldir.in

QMAKE_POST_LINK += $$QMAKE_COPY $${QMLDIR_FILES} $$DESTDIR

target.path = $${PLUGIN_INSTALL_BASE}
INSTALLS += target

qmldir.files = qmldir
qmldir.path = $${PLUGIN_INSTALL_BASE}
INSTALLS += qmldir
