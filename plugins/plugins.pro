TEMPLATE = subdirs
SUBDIRS = \
    OnlineAccountsPlugin \
    module \
    example

module.depends = OnlineAccountsPlugin
