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
#include "onlineaccountsui_adaptor.h"
#include "request.h"
#include "service.h"

#include <QQueue>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

typedef QQueue<Request*> RequestQueue;

class ServicePrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Service)

public:
    ServicePrivate(Service *service);
    ~ServicePrivate();

    RequestQueue &queueForWindowId(WId windowId);
    void enqueue(Request *request);
    void runQueue(RequestQueue &queue);

private Q_SLOTS:
    void onRequestCompleted();

private:
    mutable Service *q_ptr;
    /* each window Id has a different queue */
    QMap<WId,RequestQueue> m_requests;
};

} // namespace

ServicePrivate::ServicePrivate(Service *service):
    QObject(service),
    q_ptr(service)
{
}

ServicePrivate::~ServicePrivate()
{
}

RequestQueue &ServicePrivate::queueForWindowId(WId windowId)
{
    if (!m_requests.contains(windowId)) {
        RequestQueue queue;
        m_requests.insert(windowId, queue);
    }
    return m_requests[windowId];
}

void ServicePrivate::enqueue(Request *request)
{
    Q_Q(Service);
    bool wasIdle = q->isIdle();

    WId windowId = request->windowId();

    RequestQueue &queue = queueForWindowId(windowId);
    queue.enqueue(request);

    if (wasIdle) {
        Q_EMIT q->isIdleChanged();
    }

    runQueue(queue);
}

void ServicePrivate::runQueue(RequestQueue &queue)
{
    Request *request = queue.head();
    DEBUG() << "Head:" << request;

    if (request->isInProgress()) {
        DEBUG() << "Already in progress";
        return; // Nothing to do
    }

    QObject::connect(request, SIGNAL(completed()),
                     this, SLOT(onRequestCompleted()));
    request->start();
}

void ServicePrivate::onRequestCompleted()
{
    Q_Q(Service);

    Request *request = qobject_cast<Request*>(sender());
    WId windowId = request->windowId();

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

Service::Service(QObject *parent):
    QObject(parent),
    d_ptr(new ServicePrivate(this))
{
    new OnlineAccountsUiAdaptor(this);
}

Service::~Service()
{
}

bool Service::isIdle() const
{
    Q_D(const Service);
    return d->m_requests.isEmpty();
}

QVariantMap Service::requestAccess(const QVariantMap &options)
{
    Q_D(Service);

    DEBUG() << "Got request:" << options;

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);

    Request *request = Request::newRequest(connection(),
                                           message(),
                                           options,
                                           this);
    if (request) {
        d->enqueue(request);
    } else {
        sendErrorReply(OAU_ERROR_INVALID_PARAMETERS,
                       QStringLiteral("Invalid request"));
    }

    return QVariantMap();
}

#include "service.moc"
