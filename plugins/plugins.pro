TEMPLATE = subdirs
SUBDIRS = \
    OnlineAccountsPlugin \
    exec-tool \
    module \
    example

module.depends = OnlineAccountsPlugin
