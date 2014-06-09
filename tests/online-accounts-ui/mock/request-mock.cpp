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

#include "globals.h"
#include "request-mock.h"

#include <QDebug>

using namespace OnlineAccountsUi;

RequestPrivate::RequestPrivate(const QDBusConnection &connection,
                               const QDBusMessage &message,
                               const QVariantMap &parameters,
                               Request *request):
    QObject(request),
    q_ptr(request),
    m_parameters(parameters),
    m_window(0),
    m_inProgress(false)
{
    Q_UNUSED(connection);
    Q_UNUSED(message);
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
}

Request::~Request()
{
}

void Request::setWindow(QWindow *window)
{
    Q_D(Request);
    Q_EMIT d->setWindowCalled(window);
}

WId Request::windowId() const
{
    Q_D(const Request);
    return d->m_parameters[OAU_KEY_WINDOW_ID].toUInt();
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

QWindow *Request::window() const
{
    Q_D(const Request);
    return d->m_window;
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
    Q_EMIT d->failCalled(name, message);

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
        Q_EMIT d->setResultCalled(result);

        Q_EMIT completed();
        d->m_inProgress = false;
    }
}
