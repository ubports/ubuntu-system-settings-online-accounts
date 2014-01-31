include(../common-project-config.pri)

TEMPLATE = aux

hooks.files = \
    account-application.hook
hooks.path = $${INSTALL_PREFIX}/share/click/hooks
INSTALLS += hooks
