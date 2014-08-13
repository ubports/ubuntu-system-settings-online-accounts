include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE=aux

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
    ProvidersList.qml

settings.files = online-accounts.settings
settings.path = $${PLUGIN_MANIFEST_DIR}
INSTALLS += settings

image.files = settings-accounts.svg
image.path = $${PLUGIN_MANIFEST_DIR}/icons
INSTALLS += image

qml.files = $${QML_SOURCES}
qml.path = $${PLUGIN_QML_DIR}/online-accounts
INSTALLS += qml
