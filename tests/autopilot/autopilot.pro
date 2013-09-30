include(../../common-project-config.pri)

TEMPLATE = aux

autopilot.path = $${INSTALL_PREFIX}/lib/python2.7/dist-packages/
autopilot.files = online_accounts_ui
INSTALLS = autopilot
