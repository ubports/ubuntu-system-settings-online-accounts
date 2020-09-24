include(../../common-project-config.pri)

TEMPLATE = aux

QMAKE_EXTRA_TARGETS = autopilot service provider application

autopilot.path = $${INSTALL_PREFIX}/lib/python3/dist-packages/
autopilot.files = online_accounts_ui
INSTALLS += autopilot

service.path = $${INSTALL_PREFIX}/share/accounts/services
service.files = \
    data/ussoa-fake-service.service \
    data/ussoa-test-login-photos.service
INSTALLS += service

provider.path = $${INSTALL_PREFIX}/share/accounts/providers
provider.files = \
    data/ussoa-fake-oauth.provider \
    data/ussoa-test-login.provider
INSTALLS += provider

application.path = $${INSTALL_PREFIX}/share/accounts/applications
application.files = \
    data/ussoa-integration-tests.application
INSTALLS += application
