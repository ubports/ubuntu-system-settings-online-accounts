/*
 * Copyright (C) 2014 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of online-accounts-ui
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OAU_LIBACCOUNTS_SERVICE_H
#define OAU_LIBACCOUNTS_SERVICE_H

#include <QDBusContext>
#include <QDBusMessage>
#include <QObject>

namespace OnlineAccountsUi {

#define LIBACCOUNTS_OBJECT_PATH \
    QStringLiteral("/com/google/code/AccountsSSO/Accounts/Manager")
#define LIBACCOUNTS_BUS_NAME \
    QStringLiteral("com.google.code.AccountsSSO.Accounts.Manager")

class LibaccountsServicePrivate;

class LibaccountsService: public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface",
                "com.google.code.AccountsSSO.Accounts.Manager")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.google.code.AccountsSSO.Accounts.Manager\">\n"
"    <method name=\"store\">\n"
"      <arg direction=\"in\" type=\"u\" name=\"account_id\"/>\n"
"      <arg direction=\"in\" type=\"b\"/>\n"
"      <arg direction=\"in\" type=\"b\"/>\n"
"      <arg direction=\"in\" type=\"s\"/>\n"
"      <arg direction=\"in\" type=\"a(ssua{sv}as)\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")

public:
    explicit LibaccountsService(QObject *parent = 0);
    ~LibaccountsService();

public Q_SLOTS:
    void store(const QDBusMessage &msg);

private:
    LibaccountsServicePrivate *d_ptr;
    Q_DECLARE_PRIVATE(LibaccountsService)
};

} // namespace

#endif // OAU_LIBACCOUNTS_SERVICE_H
