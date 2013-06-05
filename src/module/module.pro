include(../../common-project-config.pri)

TEMPLATE = lib
TARGET = OnlineAccountsSetup

API_URI = "Ubuntu.OnlineAccounts.Setup"

PLUGIN_INSTALL_BASE = $$[QT_INSTALL_QML]/$$replace(API_URI, \\., /)

QML_SOURCES = \
    OAuthMain.qml \
    OAuth.qml \
    Options.qml \
    RemovalConfirmation.qml \
    ServiceItem.qml \
    ServiceSwitches.qml

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
