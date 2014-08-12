include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = online-accounts

CONFIG += \
    link_pkgconfig \
    plugin \
    qt

QT += \
    core \
    qml

QML_SOURCES = \
  AccountEditPage.qml \
  AccountItem.qml \
  AccountsPage.qml \
  AddAccountLabel.qml \
  MainPage.qml \
  NewAccountPage.qml \
  NoAccountsPage.qml \
  NormalStartupPage.qml \
  ProviderPluginList.qml \
  ProvidersList.qml \

PKGCONFIG += \
    SystemSettings

LIBS += -lonline-accounts-client
QMAKE_LIBDIR += $${TOP_BUILD_DIR}/client/OnlineAccountsClient
INCLUDEPATH += \
    $${TOP_SRC_DIR}/client \
    /usr/include

SOURCES += \
    plugin.cpp

HEADERS += \
    plugin.h

settings.files = online-accounts.settings
settings.path = $${PLUGIN_MANIFEST_DIR}
INSTALLS += settings

target.path = $${PLUGIN_MODULE_DIR}
INSTALLS += target

image.files = settings-accounts.svg
image.path = $${PLUGIN_MANIFEST_DIR}/icons
INSTALLS += image

qml.files = $${QML_SOURCES}
qml.path = $${PLUGIN_QML_DIR}/online-accounts
INSTALLS += qml
