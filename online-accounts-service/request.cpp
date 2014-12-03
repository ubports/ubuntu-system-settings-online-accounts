/*
 * Copyright (C) 2013 Canonical Ltd.
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
#include "globals.h"
#include "request.h"

#include <SignOn/uisessiondata_priv.h>

using namespace OnlineAccountsUi;

static bool mapIsSuperset(const QVariantMap &test, const QVariantMap &set)
{
    QMapIterator<QString, QVariant> it(set);
    while (it.hasNext()) {
        it.next();
        if (test.value(it.key()) != it.value()) return false;
    }

    return true;
}

namespace OnlineAccountsUi {

static QList<Request *> allRequests;

class RequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Request)

public:
    RequestPrivate(const QDBusConnection &connection,
                   const QDBusMessage &message,
                   const QVariantMap &parameters,
                   Request *request);
    ~RequestPrivate();

    quint64 windowId() const {
        return m_parameters[OAU_KEY_WINDOW_ID].toUInt();
    }

private:
    QString findClientApparmorProfile();

private:
    mutable Request *q_ptr;
    QDBusConnection m_connection;
    QDBusMessage m_message;
    QVariantMap m_parameters;
    QString m_clientApparmorProfile;
    bool m_inProgress;
    int m_delay;
};

} // namespace

RequestPrivate::RequestPrivate(const QDBusConnection &connection,
                               const QDBusMessage &message,
                               const QVariantMap &parameters,
                               Request *request):
    QObject(request),
    q_ptr(request),
    m_connection(connection),
    m_message(message),
    m_parameters(parameters),
    m_inProgress(false),
    m_delay(0)
{
    m_clientApparmorProfile = findClientApparmorProfile();
}

RequestPrivate::~RequestPrivate()
{
}

QString RequestPrivate::findClientApparmorProfile()
{
    QString uniqueConnectionId = m_message.service();
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

Request::Request(const QDBusConnection &connection,
                 const QDBusMessage &message,
                 const QVariantMap &parameters,
                 QObject *parent):
    QObject(parent),
    d_ptr(new RequestPrivate(connection, message, parameters, this))
{
    allRequests.append(this);
}

Request::~Request()
{
    allRequests.removeOne(this);
}

Request *Request::find(const QVariantMap &match)
{
    Q_FOREACH(Request *r, allRequests) {
        if (mapIsSuperset(r->parameters(), match)) {
            return r;
        }
    }

    return 0;
}

quint64 Request::windowId() const
{
    Q_D(const Request);
    return d->windowId();
}

pid_t Request::clientPid() const
{
    Q_D(const Request);
    if (interface() == OAU_INTERFACE) {
        return d->m_parameters.value(OAU_KEY_PID, 0).toUInt();
    } else if (interface() == SIGNONUI_INTERFACE) {
        return d->m_parameters.value(SSOUI_KEY_PID, 0).toUInt();
    }

    return 0;
}

void Request::setInProgress(bool inProgress)
{
    Q_D(Request);
    d->m_inProgress = inProgress;
}

bool Request::isInProgress() const
{
    Q_D(const Request);
    return d->m_inProgress;
}

const QVariantMap &Request::parameters() const
{
    Q_D(const Request);
    return d->m_parameters;
}

QString Request::clientApparmorProfile() const
{
    Q_D(const Request);
    return d->m_clientApparmorProfile;
}

QString Request::interface() const {
    Q_D(const Request);
    return d->m_message.interface();
}

QString Request::providerId() const
{
    Q_D(const Request);
    if (interface() == OAU_INTERFACE) {
        return d->m_parameters.value(OAU_KEY_PROVIDER, 0).toString();
    } else {
        return QString();
    }
}

void Request::setDelay(int delay)
{
    Q_D(Request);
    d->m_delay = delay;
}

int Request::delay() const
{
    Q_D(const Request);
    return d->m_delay;
}

void Request::cancel()
{
    setCanceled();
}

void Request::fail(const QString &name, const QString &message)
{
    Q_D(Request);
    QDBusMessage reply = d->m_message.createErrorReply(name, message);
    d->m_connection.send(reply);

    Q_EMIT completed();
}

void Request::setCanceled()
{
    Q_D(Request);
    if (d->m_inProgress) {
        fail(OAU_ERROR_USER_CANCELED, QStringLiteral("Canceled"));
        d->m_inProgress = false;
    }
}

void Request::setResult(const QVariantMap &result)
{
    Q_D(Request);
    if (d->m_inProgress) {
        QDBusMessage reply = d->m_message.createReply(result);
        d->m_connection.send(reply);

        Q_EMIT completed();
        d->m_inProgress = false;
    }
}

#include "request.moc"
