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
#include <QDBusMessage>

namespace OnlineAccountsUi {

QString apparmorProfileOfPeer(const QDBusMessage &message)
{
    QString uniqueConnectionId = message.service();
    /* This is mainly for unit tests: real messages on the session bus always
     * have a service name. */
    if (uniqueConnectionId.isEmpty()) return QString();

    QString appId;

    QDBusMessage msg =
        QDBusMessage::createMethodCall("org.freedesktop.DBus",
                                       "/org/freedesktop/DBus",
                                       "org.freedesktop.DBus",
                                       "GetConnectionAppArmorSecurityContext");
    QVariantList args;
    args << uniqueConnectionId;
    msg.setArguments(args);
    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        appId = reply.arguments().value(0, QString()).toString();
        DEBUG() << "App ID:" << appId;
    } else {
        qWarning() << "Error getting app ID:" << reply.errorName() <<
            reply.errorMessage();
    }
    return appId;
}

} // namespace
