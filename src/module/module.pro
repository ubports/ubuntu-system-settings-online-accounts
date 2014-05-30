include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = OnlineAccountsPlugin

API_URI = "Ubuntu.OnlineAccounts.Plugin"

PLUGIN_INSTALL_BASE = $${PLUGIN_PRIVATE_MODULE_DIR}/$$replace(API_URI, \\., /)

QML_SOURCES = \
    KeyboardRectangle.qml \
    OAuthMain.qml \
    OAuth.qml \
    Options.qml \
    RemovalConfirmation.qml \
    ServiceItem.qml \
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

qmldir.files = qmldir
qmldir.path = $${PLUGIN_INSTALL_BASE}
INSTALLS += qmldir

pkgconfig.files = $${TARGET}.pc
include($${TOP_SRC_DIR}/common-pkgconfig.pri)
