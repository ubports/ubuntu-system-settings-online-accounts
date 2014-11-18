include(../../common-project-config.pri)

TEMPLATE = app
TARGET = exec-tool

CONFIG += \
    qt

QT += \
    core

SOURCES += \
    main.cpp

ONLINE_ACCOUNTS_UI = "online-accounts-ui"
ONLINE_ACCOUNTS_UI_PATH = $${INSTALL_PREFIX}/bin/$${ONLINE_ACCOUNTS_UI}

DEFINES += \
    ONLINE_ACCOUNTS_UI_PATH=\\\"$${ONLINE_ACCOUNTS_UI_PATH}\\\" \
    QT_NO_KEYWORDS

target.path = $${INSTALL_LIBDIR}/ubuntu-app-launch/$${ONLINE_ACCOUNTS_UI}
INSTALLS += target
