include(../common-project-config.pri)

TEMPLATE = app
TARGET = online-accounts-hooks

CONFIG += \
    link_pkgconfig \
    qt

QT += \
    xml

PKGCONFIG += \
    accounts-qt5

SOURCES += \
    main.cpp

DEFINES += \
    HOOK_FILES_SUBDIR=\\\"$${TARGET}\\\"

QMAKE_SUBSTITUTES += \
    account-application.hook.in \
    account-provider.hook.in \
    account-service.hook.in

hooks.files = \
    account-application.hook \
    account-provider.hook \
    account-qml-plugin.hook \
    account-service.hook
hooks.path = $${INSTALL_PREFIX}/share/click/hooks
INSTALLS += hooks

include($${TOP_SRC_DIR}/common-installs-config.pri)
