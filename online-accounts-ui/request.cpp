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
// TODO #include "dialog-request.h"
#include "globals.h"
#include "provider-request.h"
#include "request.h"
#include "signonui-request.h"

#include <QPointer>

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
    RequestPrivate(const QString &interface,
                   int id,
                   const QString &clientProfile,
                   const QVariantMap &parameters,
                   Request *request);
    ~RequestPrivate();

    WId windowId() const {
        return m_parameters[OAU_KEY_WINDOW_ID].toUInt();
    }

private:
    void setWindow(QWindow *window);

private:
    mutable Request *q_ptr;
    QString m_interface;
    int m_id;
    QVariantMap m_parameters;
    QString m_clientApparmorProfile;
    bool m_inProgress;
    QPointer<QWindow> m_window;
    QString m_errorName;
    QString m_errorMessage;
    QVariantMap m_result;
    int m_delay;
};

} // namespace

RequestPrivate::RequestPrivate(const QString &interface,
                               int id,
                               const QString &clientProfile,
                               const QVariantMap &parameters,
                               Request *request):
    QObject(request),
    q_ptr(request),
    m_interface(interface),
    m_id(id),
    m_parameters(parameters),
    m_clientApparmorProfile(clientProfile),
    m_inProgress(false),
    m_window(0),
    m_delay(0)
{
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

/* Some unit tests might need to provide a different implementation for the
 * Request::newRequest() factory method; for this reason, we allow the method
 * to be excluded from compilation.
 */
#ifndef NO_REQUEST_FACTORY
Request *Request::newRequest(const QString &interface,
                             int id,
                             const QString &clientProfile,
                             const QVariantMap &parameters,
                             QObject *parent)
{
    /* If the supported requests types vary considerably, we can create
     * different subclasses for handling them, and in this method we examine
     * the @parameters argument to figure out which subclass is the most apt to
     * handle the request. */
    if (interface == OAU_INTERFACE) {
        return new ProviderRequest(interface, id, clientProfile,
                                   parameters, parent);
    } else {
        Q_ASSERT(interface == SIGNONUI_INTERFACE);
        return SignOnUi::Request::newRequest(id, clientProfile,
                                             parameters, parent);
    }
}
#endif

Request::Request(const QString &interface,
                 int id,
                 const QString &clientProfile,
                 const QVariantMap &parameters,
                 QObject *parent):
    QObject(parent),
    d_ptr(new RequestPrivate(interface, id, clientProfile, parameters, this))
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

QString Request::interface() const
{
    Q_D(const Request);
    return d->m_interface;
}

int Request::id() const
{
    Q_D(const Request);
    return d->m_id;
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

QWindow *Request::window() const
{
    Q_D(const Request);
    return d->m_window;
}

QVariantMap Request::result() const
{
    Q_D(const Request);
    return d->m_result;
}

QString Request::errorName() const
{
    Q_D(const Request);
    return d->m_errorName;
}

QString Request::errorMessage() const
{
    Q_D(const Request);
    return d->m_errorMessage;
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

    DEBUG() << name << message;

    d->m_errorName = name;
    d->m_errorMessage = message;

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
        DEBUG() << result;
        d->m_result = result;

        Q_EMIT completed();
        d->m_inProgress = false;
    }
}

#include "request.moc"
