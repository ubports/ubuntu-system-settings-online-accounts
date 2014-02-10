include(../common-project-config.pri)

TEMPLATE = aux

hooks.files = \
    account-application.hook \
    account-provider.hook \
    account-qml-plugin.hook \
    account-service.hook
hooks.path = $${INSTALL_PREFIX}/share/click/hooks
INSTALLS += hooks
