include(../../common-project-config.pri)

QDOC = $$[QT_HOST_BINS]/qdoc

QMAKE_EXTRA_TARGETS += clean-docs docs

CONFIG(ubuntu-docs) {
    docs.commands = \
        "$${QDOC} $${PWD}/online-accounts-client-ubuntu.qdocconf"
} else {
    docs.commands = \
        "$${QDOC} $${PWD}/online-accounts-client.qdocconf"
}

docs.files = $$PWD/html
docs.path = $${INSTALL_PREFIX}/share/online-accounts-client/doc

INSTALLS += docs

clean-docs.commands = \
    "rm -rf $${PWD}/html"
