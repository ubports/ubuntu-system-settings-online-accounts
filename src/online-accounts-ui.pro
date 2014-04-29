include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = app
TARGET = online-accounts-ui

CONFIG += \
    link_pkgconfig \
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

I18N_DOMAIN="ubuntu-system-settings-online-accounts"
SIGNONUI_I18N_DOMAIN="signon-ui"

DEFINES += \
    I18N_DOMAIN=\\\"$${I18N_DOMAIN}\\\" \
    SIGNONUI_I18N_DOMAIN=\\\"$${SIGNONUI_I18N_DOMAIN}\\\"

DBUS_ADAPTORS += \
    com.ubuntu.OnlineAccountsUi.xml

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
    inactivity-timer.cpp \
    indicator-service.cpp \
    main.cpp \
    panel-request.cpp \
    provider-request.cpp \
    reauthenticator.cpp \
    request.cpp \
    request-handler.cpp \
    request-manager.cpp \
    service.cpp \
    signonui-request.cpp \
    signonui-service.cpp

HEADERS += \
    access-model.h \
    account-manager.h \
    application-manager.h \
    browser-request.h \
    debug.h \
    dialog.h \
    i18n.h \
    inactivity-timer.h \
    indicator-service.h \
    panel-request.h \
    provider-request.h \
    reauthenticator.h \
    request.h \
    request-handler.h \
    request-manager.h \
    service.h \
    signonui-request.h \
    signonui-service.h

QML_SOURCES = \
    qml/AccountCreationPage.qml \
    qml/AccountEditPage.qml \
    qml/AccountItem.qml \
    qml/AccountsPage.qml \
    qml/AddAccountLabel.qml \
    qml/AuthorizationPage.qml \
    qml/MainPage.qml \
    qml/NewAccountPage.qml \
    qml/NoAccountsPage.qml \
    qml/NormalStartupPage.qml \
    qml/ProviderPluginList.qml \
    qml/ProviderRequest.qml \
    qml/ProvidersList.qml

RESOURCES += \
    ui.qrc

OTHER_FILES += \
    $${QML_SOURCES} \
    $${RESOURCES}

QMAKE_SUBSTITUTES += \
    com.canonical.indicators.webcredentials.service.in \
    com.ubuntu.OnlineAccountsUi.service.in \
    online-accounts-ui.desktop.in

DBUS_ADAPTORS += \
    com.canonical.indicators.webcredentials.xml

service.path = $${INSTALL_PREFIX}/share/dbus-1/services
service.files = \
    com.canonical.indicators.webcredentials.service \
    com.nokia.singlesignonui.service \
    com.ubuntu.OnlineAccountsUi.service
INSTALLS += service

desktop.path = $${INSTALL_PREFIX}/share/applications
desktop.files += online-accounts-ui.desktop
INSTALLS += desktop

include($${TOP_SRC_DIR}/common-installs-config.pri)
