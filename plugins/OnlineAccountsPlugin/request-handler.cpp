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

#include <SignOn/uisessiondata_priv.h>
#include <QDebug>
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

class RequestHandlerWatcherPrivate
{
    Q_DECLARE_PUBLIC(RequestHandlerWatcher)

public:
    RequestHandlerWatcherPrivate(RequestHandlerWatcher *watcher);
    ~RequestHandlerWatcherPrivate();

    static RequestHandlerWatcherPrivate *instance();
    void registerHandler(RequestHandler *handler);

private:
    mutable RequestHandlerWatcher *q_ptr;
    static RequestHandlerWatcherPrivate *m_instance;
};

RequestHandlerWatcherPrivate *RequestHandlerWatcherPrivate::m_instance = 0;

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

    RequestHandlerWatcherPrivate *watcher =
        RequestHandlerWatcherPrivate::instance();
    if (Q_LIKELY(watcher)) {
        watcher->registerHandler(this);
    }
}

RequestHandler::~RequestHandler()
{
    allRequestHandlers.removeOne(this);
    delete d_ptr;
}

void RequestHandler::setRequest(QObject *request)
{
    Q_D(RequestHandler);

    if (request == d->m_request) return;

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

RequestHandlerWatcherPrivate::RequestHandlerWatcherPrivate(RequestHandlerWatcher *watcher):
    q_ptr(watcher)
{
    if (Q_UNLIKELY(m_instance)) {
        qWarning() << "RequestHandlerWatcher should be instantiated once!";
    }

    m_instance = this;
}

RequestHandlerWatcherPrivate::~RequestHandlerWatcherPrivate()
{
    m_instance = 0;
}

RequestHandlerWatcherPrivate *RequestHandlerWatcherPrivate::instance()
{
    return m_instance;
}

void RequestHandlerWatcherPrivate::registerHandler(RequestHandler *handler)
{
    Q_Q(RequestHandlerWatcher);

    Q_EMIT q->newHandler(handler);
}

RequestHandlerWatcher::RequestHandlerWatcher(QObject *parent):
    QObject(parent),
    d_ptr(new RequestHandlerWatcherPrivate(this))
{
}

RequestHandlerWatcher::~RequestHandlerWatcher()
{
    delete d_ptr;
}

RequestHandler *RequestHandlerWatcher::findMatching(const QVariantMap &parameters)
{
    /* Find if there's any RequestHandler expecting to handle the SignOnUi
     * request having "parameters" as parameters.
     * This is simply done by matching on the X-RequestHandler key (aka
     * matchKey()), if present. We expect that account plugins add that field
     * to their AuthSession requests which they want to handle themselves.
     */
    qDebug() << parameters;
    if (!parameters.contains(SSOUI_KEY_CLIENT_DATA)) return 0;
    QVariantMap clientData = parameters[SSOUI_KEY_CLIENT_DATA].toMap();
    qDebug() << "client data:" << clientData;
    QString matchId = clientData.value(RequestHandler::matchKey()).toString();
    if (matchId.isEmpty()) return 0;

    Q_FOREACH(RequestHandler *handler, allRequestHandlers) {
        if (handler->matchId() == matchId) return handler;
    }
    return 0;
}
