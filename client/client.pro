include(doc/doc.pri)

TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = \
    OnlineAccountsClient \
    module \
    tests
