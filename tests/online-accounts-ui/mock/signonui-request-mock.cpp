/*
 * This file is part of online-accounts-ui
 *
 * Copyright (C) 2011-2014 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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
#include "signonui-request-mock.h"

#include <QDebug>
#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

RequestPrivate::RequestPrivate(Request *request):
    QObject(request),
    q_ptr(request),
    m_handler(0)
{
    const QVariantMap &parameters = request->parameters();
    if (parameters.contains(SSOUI_KEY_CLIENT_DATA)) {
        m_clientData = parameters[SSOUI_KEY_CLIENT_DATA].toMap();
    }
}

RequestPrivate::~RequestPrivate()
{
}

Request::Request(int id,
                 const QString &clientProfile,
                 const QVariantMap &parameters,
                 QObject *parent):
    OnlineAccountsUi::Request(SIGNONUI_INTERFACE, id, clientProfile,
                              parameters, parent),
    d_ptr(new RequestPrivate(this))
{
}

Request::~Request()
{
}

QString Request::ssoId(const QVariantMap &parameters)
{
    return parameters[SSOUI_KEY_REQUESTID].toString();
}

QString Request::ssoId() const
{
    return Request::ssoId(parameters());
}

void Request::setWindow(QWindow *window)
{
    OnlineAccountsUi::Request::setWindow(window);
}

uint Request::identity() const
{
    return parameters().value(SSOUI_KEY_IDENTITY).toUInt();
}

QString Request::method() const
{
    return parameters().value(SSOUI_KEY_METHOD).toString();
}

QString Request::mechanism() const
{
    return parameters().value(SSOUI_KEY_MECHANISM).toString();
}

QString Request::providerId() const
{
    Q_D(const Request);
    return d->m_providerId;
}

const QVariantMap &Request::clientData() const
{
    Q_D(const Request);
    return d->m_clientData;
}

void Request::setHandler(RequestHandler *handler)
{
    Q_D(Request);
    d->m_handler = handler;
}

RequestHandler *Request::handler() const
{
    Q_D(const Request);
    return d->m_handler;
}

void Request::setCanceled()
{
    QVariantMap result;
    result[SSOUI_KEY_ERROR] = SignOn::QUERY_ERROR_CANCELED;

    setResult(result);
}
