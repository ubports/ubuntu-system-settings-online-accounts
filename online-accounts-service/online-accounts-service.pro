include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = app
TARGET = online-accounts-service

CONFIG += \
    link_pkgconfig \
    no_keywords \
    qt

QT += \
    dbus \
    network

PKGCONFIG += \
    libnotify \
    libsignon-qt5 \
    mirclient \
    signon-plugins-common

DBUS_ADAPTORS += \
    com.ubuntu.OnlineAccountsUi.xml

DEFINES += \
    DEBUG_ENABLED \
    SIGNONUI_I18N_DOMAIN=\\\"$${SIGNONUI_I18N_DOMAIN}\\\"

COMMON_SRC = ../online-accounts-ui

INCLUDEPATH += \
    $${COMMON_SRC}

SOURCES += \
    $${COMMON_SRC}/debug.cpp \
    $${COMMON_SRC}/i18n.cpp \
    $${COMMON_SRC}/ipc.cpp \
    $${COMMON_SRC}/notification.cpp \
    inactivity-timer.cpp \
    indicator-service.cpp \
    main.cpp \
    mir-helper.cpp \
    reauthenticator.cpp \
    request.cpp \
    request-manager.cpp \
    service.cpp \
    signonui-service.cpp \
    ui-proxy.cpp

HEADERS += \
    $${COMMON_SRC}/debug.h \
    $${COMMON_SRC}/i18n.h \
    $${COMMON_SRC}/ipc.h \
    $${COMMON_SRC}/notification.h \
    inactivity-timer.h \
    indicator-service.h \
    mir-helper.h \
    reauthenticator.h \
    request.h \
    request-manager.h \
    service.h \
    signonui-service.h \
    ui-proxy.h

QMAKE_SUBSTITUTES += \
    com.ubuntu.OnlineAccountsUi.service.in

DBUS_ADAPTORS += \
    com.canonical.indicators.webcredentials.xml

service.path = $${INSTALL_PREFIX}/share/dbus-1/services
service.files = \
    com.ubuntu.OnlineAccountsUi.service
INSTALLS += service

include($${TOP_SRC_DIR}/common-installs-config.pri)
