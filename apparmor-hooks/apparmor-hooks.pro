include(../common-project-config.pri)

TEMPLATE = aux

apparmor.files = \
    account-application.hook
apparmor.path = $${INSTALL_PREFIX}/share/click/hooks
INSTALLS += apparmor
