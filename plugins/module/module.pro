include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = OnlineAccountsPlugin

API_URI = "Ubuntu.OnlineAccounts.Plugin"
API_VER = 1.0

PLUGIN_INSTALL_BASE = $${PLUGIN_PRIVATE_MODULE_DIR}/$$replace(API_URI, \\., /)

CONFIG += \
    link_pkgconfig \
    plugin \
    qt

QT += qml

PKGCONFIG += \
    accounts-qt5

# Error on undefined symbols
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES = \
    plugin.cpp

HEADERS += \
    plugin.h

INCLUDEPATH += \
    $$TOP_SRC_DIR/plugins
QMAKE_LIBDIR = $${TOP_BUILD_DIR}/plugins/OnlineAccountsPlugin
LIBS += -lonline-accounts-plugin

QML_SOURCES = \
    ChromedWebView.qml \
    DuplicateAccount.qml \
    ErrorItem.qml \
    KeyboardRectangle.qml \
    OAuthMain.qml \
    OAuth.qml \
    Options.qml \
    RemovalConfirmation.qml \
    ServiceItem.qml \
    ServiceItemBase.qml \
    ServiceSwitches.qml \
    StandardAnimation.qml \
    WebView.qml

OTHER_FILES += $${QML_SOURCES}

qml.files = $${QML_SOURCES}
qml.path = $${PLUGIN_INSTALL_BASE}
INSTALLS += qml

QMLDIR_FILES += qmldir
QMAKE_SUBSTITUTES += qmldir.in
OTHER_FILES += qmldir.in

target.path = $${PLUGIN_INSTALL_BASE}
INSTALLS += target

qmldir.files = qmldir
qmldir.path = $${PLUGIN_INSTALL_BASE}
INSTALLS += qmldir
