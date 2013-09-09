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

DEFINES += \
    I18N_DOMAIN=\\\"ubuntu-system-settings-online-accounts\\\"

DBUS_ADAPTORS += \
    com.canonical.OnlineAccountsUi.xml

DEFINES += \
    DEBUG_ENABLED \
    OAU_PLUGIN_DIR=\\\"$${ONLINE_ACCOUNTS_PLUGIN_DIR}/\\\" \
    PLUGIN_PRIVATE_MODULE_DIR=\\\"$${PLUGIN_PRIVATE_MODULE_DIR}\\\"

SOURCES += \
    debug.cpp \
    i18n.cpp \
    inactivity-timer.cpp \
    main.cpp \
    panel-request.cpp \
    request.cpp \
    service.cpp

HEADERS += \
    debug.h \
    i18n.h \
    inactivity-timer.h \
    panel-request.h \
    request.h \
    service.h

QML_SOURCES = \
    qml/AccountCreationPage.qml \
    qml/AccountEditPage.qml \
    qml/AccountItem.qml \
    qml/AccountsPage.qml \
    qml/AddAccountLabel.qml \
    qml/MainPage.qml \
    qml/NewAccountPage.qml \
    qml/NoAccountsPage.qml \
    qml/NormalStartupPage.qml \
    qml/ProvidersList.qml

RESOURCES += \
    ui.qrc

OTHER_FILES += \
    $${QML_SOURCES} \
    $${RESOURCES}

QMAKE_SUBSTITUTES += \
    com.canonical.OnlineAccountsUi.service.in

service.path = $${INSTALL_PREFIX}/share/dbus-1/services
service.files = \
    com.canonical.OnlineAccountsUi.service
INSTALLS += service

include($${TOP_SRC_DIR}/common-installs-config.pri)

