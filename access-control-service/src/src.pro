include (../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = app
TARGET = access-control-service

CONFIG += \
    link_pkgconfig \
    qt

QT += \
    dbus \
    gui

QMAKE_CXXFLAGS += \
    -fvisibility=hidden

PKGCONFIG += \
    accounts-qt5 \
    libsignon-qt5

DEFINES += \
    I18N_DOMAIN=\\\"$${TARGET}\\\"

DBUS_ADAPTORS += \
    ../com.canonical.OnlineAccounts.AccessControl.xml

SOURCES += \
    account-manager.cpp \
    debug.cpp \
    i18n.cpp \
    inactivity-timer.cpp \
    main.cpp \
    provider-request.cpp \
    request.cpp \
    service.cpp

HEADERS += \
    account-manager.h \
    debug.h \
    i18n.h \
    inactivity-timer.h \
    provider-request.h \
    request.h \
    service.h

include($${TOP_SRC_DIR}/common-installs-config.pri)
