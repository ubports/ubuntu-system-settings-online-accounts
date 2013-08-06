include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = OnlineAccountsClient

API_URI = "Ubuntu.OnlineAccounts.Client"
API_VER = 0.1

DESTDIR = $$replace(API_URI, \\., /).$$API_VER
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
LIBS += -lOnlineAccountsClient

QMLDIR_FILES += qmldir
QMAKE_SUBSTITUTES += qmldir.in
OTHER_FILES += qmldir.in

copy2build.output = $${DESTDIR}/${QMAKE_FILE_IN}
copy2build.input = QMLDIR_FILES
copy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
copy2build.name = COPY ${QMAKE_FILE_IN}
copy2build.variable_out = PRE_TARGETDEPS
copy2build.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += copy2build

target.path = $${PLUGIN_INSTALL_BASE}
INSTALLS += target

qmldir.files = qmldir
qmldir.path = $${PLUGIN_INSTALL_BASE}
INSTALLS += qmldir
