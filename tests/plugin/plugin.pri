include(../../common-project-config.pri)

CONFIG += \
    debug

QT += \
    core \
    testlib

DEFINES += \
    DEBUG_ENABLED

ONLINE_ACCOUNTS_PLUGIN_DIR = $${TOP_SRC_DIR}/plugins/OnlineAccountsPlugin
COMMON_SRC_DIR = $${TOP_SRC_DIR}/online-accounts-ui

INCLUDEPATH += \
    $${ONLINE_ACCOUNTS_PLUGIN_DIR} \
    $${COMMON_SRC_DIR}
