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
#include "request-manager.h"
#include "ui-proxy.h"

#include <QQueue>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

static RequestManager *m_instance = 0;

typedef QQueue<Request*> RequestQueue;

class RequestManagerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(RequestManager)

public:
    RequestManagerPrivate(RequestManager *service);
    ~RequestManagerPrivate();

    RequestQueue &queueForWindowId(quint64 windowId);
    void enqueue(Request *request);
    void runQueue(RequestQueue &queue);

private Q_SLOTS:
    void onRequestCompleted();
    void onProxyFinished();

private:
    mutable RequestManager *q_ptr;
    /* each window Id has a different queue */
    QMap<quint64,RequestQueue> m_requests;
    QList<UiProxy*> m_proxies;
};

} // namespace

RequestManagerPrivate::RequestManagerPrivate(RequestManager *service):
    QObject(service),
    q_ptr(service)
{
}

RequestManagerPrivate::~RequestManagerPrivate()
{
}

RequestQueue &RequestManagerPrivate::queueForWindowId(quint64 windowId)
{
    if (!m_requests.contains(windowId)) {
        RequestQueue queue;
        m_requests.insert(windowId, queue);
    }
    return m_requests[windowId];
}

void RequestManagerPrivate::enqueue(Request *request)
{
    Q_Q(RequestManager);

    /* First, see if any of the existing proxies can handle this request */
    Q_FOREACH(UiProxy *proxy, m_proxies) {
        if (proxy->hasHandlerFor(request->parameters())) {
            QObject::connect(request, SIGNAL(completed()),
                             request, SLOT(deleteLater()));
            proxy->handleRequest(request);
            return;
        }
    }

    bool wasIdle = q->isIdle();

    quint64 windowId = request->windowId();

    RequestQueue &queue = queueForWindowId(windowId);
    queue.enqueue(request);

    if (wasIdle) {
        Q_EMIT q->isIdleChanged();
    }

    runQueue(queue);
}

void RequestManagerPrivate::runQueue(RequestQueue &queue)
{
    Request *request = queue.head();
    DEBUG() << "Head:" << request;

    if (request->isInProgress()) {
        DEBUG() << "Already in progress";
        return; // Nothing to do
    }

    UiProxy *proxy = new UiProxy(request->clientPid(), this);
    if (Q_UNLIKELY(!proxy->init())) {
        qWarning() << "UiProxy initialization failed!";
        runQueue(queue);
        return;
    }
    QObject::connect(request, SIGNAL(completed()),
                     this, SLOT(onRequestCompleted()));
    QObject::connect(proxy, SIGNAL(finished()),
                     this, SLOT(onProxyFinished()));
    m_proxies.append(proxy);
    proxy->handleRequest(request);
}

void RequestManagerPrivate::onRequestCompleted()
{
    Q_Q(RequestManager);

    Request *request = qobject_cast<Request*>(sender());
    quint64 windowId = request->windowId();

    RequestQueue &queue = queueForWindowId(windowId);
    if (request != queue.head()) {
        qCritical("Completed request is not first in queue!");
        return;
    }

    queue.dequeue();
    request->deleteLater();

    if (queue.isEmpty()) {
        m_requests.remove(windowId);
    } else {
        /* start the next request */
        runQueue(queue);
    }

    if (q->isIdle()) {
        Q_EMIT q->isIdleChanged();
    }
}

void RequestManagerPrivate::onProxyFinished()
{
    UiProxy *proxy = qobject_cast<UiProxy*>(sender());
    m_proxies.removeOne(proxy);

    proxy->deleteLater();
}

RequestManager::RequestManager(QObject *parent):
    QObject(parent),
    d_ptr(new RequestManagerPrivate(this))
{
    if (m_instance == 0) {
        m_instance = this;
    } else {
        qWarning() << "Instantiating a second RequestManager!";
    }
}

RequestManager::~RequestManager()
{
}

RequestManager *RequestManager::instance()
{
    return m_instance;
}

void RequestManager::enqueue(Request *request)
{
    Q_D(RequestManager);
    d->enqueue(request);
}

bool RequestManager::isIdle() const
{
    Q_D(const RequestManager);
    return d->m_requests.isEmpty();
}


#include "request-manager.moc"
