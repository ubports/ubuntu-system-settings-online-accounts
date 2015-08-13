include(../common-project-config.pri)

TEMPLATE = app
TARGET = online-accounts-hooks2

CONFIG += \
    link_pkgconfig \
    qt

QT += \
    xml

PKGCONFIG += \
    accounts-qt5 \
    click-0.4 \
    gobject-2.0 \
    libsignon-qt5

SOURCES += \
    accounts.cpp \
    acl-updater.cpp

HEADERS += \
    acl-updater.h

DEFINES += \
    HOOK_FILES_SUBDIR=\\\"$${TARGET}\\\" \
    QT_NO_KEYWORDS

QMAKE_SUBSTITUTES += \
    accounts.hook.in

hooks.files = \
    accounts.hook
hooks.path = $${INSTALL_PREFIX}/share/click/hooks
INSTALLS += hooks

include($${TOP_SRC_DIR}/common-installs-config.pri)
