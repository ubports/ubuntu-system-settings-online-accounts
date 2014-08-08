include(../../common-project-config.pri)

CONFIG += \
    debug

QT += \
    core \
    testlib

DEFINES += \
    DEBUG_ENABLED

ONLINE_ACCOUNTS_UI_DIR = $${TOP_SRC_DIR}/online-accounts-ui
COMMON_SRC_DIR = $${TOP_SRC_DIR}/online-accounts-ui

INCLUDEPATH += \
    $${ONLINE_ACCOUNTS_SERVICE_DIR} \
    $${COMMON_SRC_DIR}
