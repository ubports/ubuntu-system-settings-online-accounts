include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = app
TARGET = online-accounts-ui

CONFIG += \
    link_pkgconfig \
    no_keywords \
    qt

QT += \
    dbus \
    gui \
    qml \
    quick

PKGCONFIG += \
    accounts-qt5 \
    libnotify \
    libsignon-qt5 \
    signon-plugins-common

DEFINES += \
    I18N_DOMAIN=\\\"$${I18N_DOMAIN}\\\" \
    SIGNONUI_I18N_DOMAIN=\\\"$${SIGNONUI_I18N_DOMAIN}\\\"

DEFINES += \
    DEBUG_ENABLED \
    OAU_PLUGIN_DIR=\\\"$${ONLINE_ACCOUNTS_PLUGIN_DIR}/\\\" \
    PLUGIN_PRIVATE_MODULE_DIR=\\\"$${PLUGIN_PRIVATE_MODULE_DIR}\\\"

SOURCES += \
    access-model.cpp \
    account-manager.cpp \
    application-manager.cpp \
    browser-request.cpp \
    debug.cpp \
    dialog.cpp \
    i18n.cpp \
    ipc.cpp \
    main.cpp \
    notification.cpp \
    panel-request.cpp \
    provider-request.cpp \
    request.cpp \
    request-handler.cpp \
    signonui-request.cpp \
    ui-server.cpp

HEADERS += \
    access-model.h \
    account-manager.h \
    application-manager.h \
    browser-request.h \
    debug.h \
    dialog.h \
    i18n.h \
    ipc.h \
    notification.h \
    panel-request.h \
    provider-request.h \
    request.h \
    request-handler.h \
    signonui-request.h \
    ui-server.h

QML_SOURCES = \
    qml/AccountCreationPage.qml \
    qml/AuthorizationPage.qml \
    qml/ProviderRequest.qml \
    qml/SignOnUiPage.qml

RESOURCES += \
    ui.qrc

OTHER_FILES += \
    $${QML_SOURCES} \
    $${RESOURCES}

QMAKE_SUBSTITUTES += \
    online-accounts-ui.desktop.in

desktop.path = $${INSTALL_PREFIX}/share/applications
desktop.files += online-accounts-ui.desktop
INSTALLS += desktop

include($${TOP_SRC_DIR}/common-installs-config.pri)
