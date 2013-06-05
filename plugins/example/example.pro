include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = lib
TARGET = example

QML_SOURCES = \
    Main.qml

OTHER_FILES += \
    $${QML_SOURCES} \
    example.provider

provider.files = example.provider
provider.path = $$system("pkg-config --define-variable=prefix=$${INSTALL_PREFIX} --variable providerfilesdir libaccounts-glib")
INSTALLS += provider

qml.files = $${QML_SOURCES}
qml.path = $${ONLINE_ACCOUNTS_PLUGIN_DIR}/$${TARGET}
INSTALLS += qml
