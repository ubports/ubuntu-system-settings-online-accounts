/*
 * This file is part of online-accounts-ui
 *
 * Copyright (C) 2014 Canonical Ltd.
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

#include "request-handler.h"

#include "debug.h"

#include <SignOn/uisessiondata_priv.h>
#include <QDBusArgument>
#include <QList>
#include <QPointer>
#include <unistd.h>

using namespace SignOnUi;

namespace SignOnUi {

static QList<RequestHandler *> allRequestHandlers;
static int counter = 1;

class RequestHandlerPrivate
{
    Q_DECLARE_PUBLIC(RequestHandler)

public:
    RequestHandlerPrivate(RequestHandler *request);
    ~RequestHandlerPrivate();

private:
    QString m_matchId;
    QPointer<QObject> m_request;
    mutable RequestHandler *q_ptr;
};

} // namespace

RequestHandlerPrivate::RequestHandlerPrivate(RequestHandler *request):
    q_ptr(request)
{
    m_matchId = QString("%1-%2").arg(getpid()).arg(counter++);
}

RequestHandlerPrivate::~RequestHandlerPrivate()
{
}

RequestHandler::RequestHandler(QObject *parent):
    QObject(parent),
    d_ptr(new RequestHandlerPrivate(this))
{
    allRequestHandlers.append(this);
}

RequestHandler::~RequestHandler()
{
    allRequestHandlers.removeOne(this);
    delete d_ptr;
}

void RequestHandler::setRequest(QObject *request)
{
    Q_D(RequestHandler);

    if (d->m_request != 0 && request != 0) {
        qCritical() << "RequestHandler is already assigned a request";
        return;
    }
    d->m_request = request;
    Q_EMIT requestChanged();
}

QObject *RequestHandler::request() const
{
    Q_D(const RequestHandler);
    return d->m_request;
}

QString RequestHandler::matchId() const
{
    Q_D(const RequestHandler);
    return d->m_matchId;
}

RequestHandler *RequestHandler::findMatching(const QVariantMap &parameters)
{
    /* Find if there's any RequestHandler expecting to handle the SignOnUi
     * request having "parameters" as parameters.
     * This is simply done by matching on the X-RequestHandler key (aka
     * matchKey()), if present. We expect that account plugins add that field
     * to their AuthSession requests which they want to handle themselves.
     */
    DEBUG() << parameters;
    if (!parameters.contains(SSOUI_KEY_CLIENT_DATA)) return 0;
    QVariant variant = parameters[SSOUI_KEY_CLIENT_DATA];
    QVariantMap clientData = (variant.type() == QVariant::Map) ?
        variant.toMap() :
        qdbus_cast<QVariantMap>(variant.value<QDBusArgument>());
    DEBUG() << "client data:" << clientData;
    QString matchId = clientData.value(RequestHandler::matchKey()).toString();
    if (matchId.isEmpty()) return 0;

    Q_FOREACH(RequestHandler *handler, allRequestHandlers) {
        if (handler->matchId() == matchId) return handler;
    }
    return 0;
}
