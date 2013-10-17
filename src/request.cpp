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
#include "panel-request.h"
#include "provider-request.h"
#include "request.h"

#include <QPointer>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

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

    WId windowId() const {
        return m_parameters[OAU_KEY_WINDOW_ID].toUInt();
    }

private:
    void setWindow(QWindow *window);
    QString findClientApparmorProfile();

private:
    mutable Request *q_ptr;
    QDBusConnection m_connection;
    QDBusMessage m_message;
    QVariantMap m_parameters;
    QString m_clientApparmorProfile;
    bool m_inProgress;
    QPointer<QWindow> m_window;
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
    m_window(0)
{
    m_clientApparmorProfile = findClientApparmorProfile();
}

RequestPrivate::~RequestPrivate()
{
}

void RequestPrivate::setWindow(QWindow *window)
{
    if (m_window != 0) {
        qWarning() << "Widget already set";
        return;
    }

    m_window = window;

    if (windowId() != 0) {
        DEBUG() << "Requesting window reparenting";
        QWindow *parent = QWindow::fromWinId(windowId());
        window->setTransientParent(parent);
    }
    window->show();
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

/* Some unit tests might need to provide a different implementation for the
 * Request::newRequest() factory method; for this reason, we allow the method
 * to be excluded from compilation.
 */
#ifndef NO_REQUEST_FACTORY
Request *Request::newRequest(const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent)
{
    /* If the supported requests types vary considerably, we can create
     * different subclasses for handling them, and in this method we examine
     * the @parameters argument to figure out which subclass is the most apt to
     * handle the request. */
    if (parameters.contains(OAU_KEY_PROVIDER)) {
        return new ProviderRequest(connection, message, parameters, parent);
    } else {
        return new PanelRequest(connection, message, parameters, parent);
    }
}
#endif

Request::Request(const QDBusConnection &connection,
                 const QDBusMessage &message,
                 const QVariantMap &parameters,
                 QObject *parent):
    QObject(parent),
    d_ptr(new RequestPrivate(connection, message, parameters, this))
{
}

Request::~Request()
{
}

void Request::setWindow(QWindow *window)
{
    Q_D(Request);
    d->setWindow(window);
}

WId Request::windowId() const
{
    Q_D(const Request);
    return d->windowId();
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

void Request::start()
{
    Q_D(Request);
    if (d->m_inProgress) {
        qWarning() << "Request already started!";
        return;
    }
    d->m_inProgress = true;
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
    fail(OAU_ERROR_USER_CANCELED, QStringLiteral("Canceled"));
}

void Request::setResult(const QVariantMap &result)
{
    Q_D(Request);
    QDBusMessage reply = d->m_message.createReply(result);
    d->m_connection.send(reply);

    Q_EMIT completed();
}

#include "request.moc"
