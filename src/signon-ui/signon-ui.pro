include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = aux

QML_SOURCES = \
    Page.qml

OTHER_FILES += $${QML_SOURCES}

qml.files = $${QML_SOURCES}
qml.path = $${INSTALL_PREFIX}/share/signon-ui/online-accounts-ui
INSTALLS += qml
