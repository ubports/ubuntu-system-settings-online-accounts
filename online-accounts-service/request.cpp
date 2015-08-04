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
#include "utils.h"

#include <Accounts/Manager>
#include <Accounts/Service>
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
    m_clientApparmorProfile = apparmorProfileOfPeer(message);
}

RequestPrivate::~RequestPrivate()
{
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
        if (d->m_clientApparmorProfile == "unconfined") {
            QVariantMap clientData =
                d->m_parameters.value(SSOUI_KEY_CLIENT_DATA).toMap();
            if (clientData.contains("requestorPid")) {
                return clientData.value("requestorPid").toUInt();
            }
        }
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
        QString providerId =
            d->m_parameters.value(OAU_KEY_PROVIDER).toString();
        if (providerId.isEmpty() &&
            d->m_parameters.contains(OAU_KEY_SERVICE_ID)) {
            Accounts::Manager manager;
            QString serviceId = d->m_parameters[OAU_KEY_SERVICE_ID].toString();
            Accounts::Service service = manager.service(serviceId);
            if (service.isValid()) {
                providerId = service.provider();
            }
        }
        return providerId;
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
