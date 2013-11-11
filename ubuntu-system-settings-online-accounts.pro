include(common-vars.pri)
include(common-project-config.pri)

TEMPLATE = subdirs
SUBDIRS = \
    apparmor-hooks \
    po \
    src \
    client \
    system-settings-plugin \
    plugins \
    tests

system-settings-plugin.depends = client
plugins.depends = src
tests.depends = src client

include(common-installs-config.pri)

DISTNAME = $${PROJECT_NAME}-$${PROJECT_VERSION}
dist.commands = "bzr export $${DISTNAME}.tar.bz2"
QMAKE_EXTRA_TARGETS += dist
