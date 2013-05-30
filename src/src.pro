include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = online-accounts

QML_SOURCES = \
    AccountCreationPage.qml \
    AccountItem.qml \
    AccountsPage.qml \
    AddAccountLabel.qml \
    MainPage.qml \
    NewAccountPage.qml \
    NoAccountsPage.qml \
    ProvidersList.qml

OTHER_FILES += \
    $${QML_SOURCES}

settings.files = online-accounts.settings
settings.path = $${PLUGIN_MANIFEST_DIR}
INSTALLS += settings

qml.files = $${QML_SOURCES}
qml.path = $${PLUGIN_QML_DIR}/$${TARGET}
INSTALLS += qml
