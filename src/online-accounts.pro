include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = online-accounts

QML_SOURCES = \
    AccountCreationPage.qml \
    AccountEditPage.qml \
    AccountItem.qml \
    AccountsPage.qml \
    AddAccountLabel.qml \
    MainPage.qml \
    NewAccountPage.qml \
    NoAccountsPage.qml \
    NormalStartupPage.qml \
    ProvidersList.qml

OTHER_FILES += \
    $${QML_SOURCES} \
    constants.js.in

QMAKE_SUBSTITUTES += constants.js.in

settings.files = online-accounts.settings
settings.path = $${PLUGIN_MANIFEST_DIR}
INSTALLS += settings

qml.files = \
    $${QML_SOURCES} \
    constants.js
qml.path = $${PLUGIN_QML_DIR}/$${TARGET}
INSTALLS += qml

image.files = settings-accounts.svg
image.path = /usr/share/settings/system/icons
INSTALLS += image

