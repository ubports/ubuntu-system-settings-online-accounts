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

#include "debug.h"
#include "utils.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusReply>
#include <sys/apparmor.h>

namespace OnlineAccountsUi {

QString apparmorProfileOfPeer(const QDBusMessage &message)
{
    static QString ourProfile;

    QString uniqueConnectionId = message.service();
    /* This is mainly for unit tests: real messages on the session bus always
     * have a service name. */
    if (uniqueConnectionId.isEmpty()) return QString();

    if (ourProfile.isEmpty()) {
        char *label = NULL;
        char *mode = NULL;
        aa_getcon(&label, &mode);
        ourProfile = QString::fromUtf8(label);
        free(label);
    }

    QString appId;

    QDBusMessage msg =
        QDBusMessage::createMethodCall("org.freedesktop.DBus",
                                       "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus",
                                       "GetConnectionCredentials");
    QVariantList args;
    args << uniqueConnectionId;
    msg.setArguments(args);
    QDBusReply<QVariantMap> reply =
        QDBusConnection::sessionBus().call(msg, QDBus::Block);
    if (reply.isValid()) {
        QVariantMap map = reply.value();
        QByteArray context = map.value("LinuxSecurityLabel").toByteArray();
        if (!context.isEmpty()) {
            aa_splitcon(context.data(), NULL);
            appId = QString::fromUtf8(context);
            if (appId == ourProfile) {
                qDebug() << "Same profile as ourselves, assuming unconfined";
                appId = "unconfined";
            }
        }
        qDebug() << "App ID:" << appId;
    } else {
        QDBusError error = reply.error();
        qWarning() << "Error getting app ID:" << error.name() <<
            error.message();
    }
    return appId;
}

} // namespace
