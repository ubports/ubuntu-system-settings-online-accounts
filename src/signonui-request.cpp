/*
 * This file is part of signon-ui
 *
 * Copyright (C) 2011 Canonical Ltd.
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

#include "signonui-request.h"

#include "browser-request.h"
#include "debug.h"
#include "globals.h"
#include "indicator-service.h"

#include <Accounts/Account>
#include <Accounts/Manager>
#include <QDBusArgument>
#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

namespace SignOnUi {

class RequestPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Request)

public:
    RequestPrivate(Request *request);
    ~RequestPrivate();

private:
    bool setWindow(QWindow *window);
    Accounts::Account *findAccount();
    bool dispatchToIndicator();

private:
    mutable Request *q_ptr;
    QVariantMap m_clientData;
    bool m_inProgress;
    RequestHandler *m_handler;
    Accounts::Manager *m_accountManager;
};

} // namespace

RequestPrivate::RequestPrivate(Request *request):
    QObject(request),
    q_ptr(request),
    m_inProgress(false),
    m_handler(0),
    m_accountManager(0)
{
    const QVariantMap &parameters = request->parameters();
    if (parameters.contains(SSOUI_KEY_CLIENT_DATA)) {
        QVariant variant = parameters[SSOUI_KEY_CLIENT_DATA];
        m_clientData = (variant.type() == QVariant::Map) ?
            variant.toMap() :
            qdbus_cast<QVariantMap>(variant.value<QDBusArgument>());
    }
}

RequestPrivate::~RequestPrivate()
{
}

bool RequestPrivate::setWindow(QWindow *window)
{
    Q_Q(Request);
    Q_UNUSED(window);

    /* If the window has no parent and the webcredentials indicator service is
     * up, dispatch the request to it. */
    if (q->windowId() == 0 && dispatchToIndicator()) {
        return true;
    }

    return false;
}

Accounts::Account *RequestPrivate::findAccount()
{
    Q_Q(Request);

    uint identity = q->identity();
    if (identity == 0)
        return 0;

    /* Find the account using this identity.
     * FIXME: there might be more than one!
     */
    if (m_accountManager == 0) {
        m_accountManager = new Accounts::Manager(this);
    }
    foreach (Accounts::AccountId accountId, m_accountManager->accountList()) {
        Accounts::Account *account = m_accountManager->account(accountId);
        if (account == 0) continue;

        QVariant value(QVariant::UInt);
        if (account->value("CredentialsId", value) != Accounts::NONE &&
            value.toUInt() == identity) {
            return account;
        }
    }

    // Not found
    return 0;
}

bool RequestPrivate::dispatchToIndicator()
{
    Q_Q(Request);

    Accounts::Account *account = findAccount();
    if (account == 0) {
        return false;
    }

    QVariantMap notification;
    notification["DisplayName"] = account->displayName();
    notification["ClientData"] = m_clientData;
    notification["Identity"] = q->identity();
    notification["Method"] = q->method();
    notification["Mechanism"] = q->mechanism();

    IndicatorService *indicator = IndicatorService::instance();
    indicator->reportFailure(account->id(), notification);

    /* the account has been reported as failing. We can now close this
     * request, and tell the application that UI interaction is forbidden.
     */
    QVariantMap result;
    result[SSOUI_KEY_ERROR] = SignOn::QUERY_ERROR_FORBIDDEN;
    q->setResult(result);
    return true;
}

Request *Request::newRequest(const QDBusConnection &connection,
                             const QDBusMessage &message,
                             const QVariantMap &parameters,
                             QObject *parent)
{
    if (parameters.contains(SSOUI_KEY_OPENURL)) {
        return new SignOnUi::BrowserRequest(connection, message,
                                            parameters, parent);
    } else {
        return 0; // TODO new DialogRequest(connection, message, parameters, parent);
    }
}

Request::Request(const QDBusConnection &connection,
                 const QDBusMessage &message,
                 const QVariantMap &parameters,
                 QObject *parent):
    OnlineAccountsUi::Request(connection, message, parameters, parent),
    d_ptr(new RequestPrivate(this))
{
}

Request::~Request()
{
}

QString Request::id(const QVariantMap &parameters)
{
    return parameters[SSOUI_KEY_REQUESTID].toString();
}

QString Request::id() const
{
    return Request::id(parameters());
}

void Request::setWindow(QWindow *window)
{
    Q_D(Request);
    if (!d->setWindow(window))
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

const QVariantMap &Request::clientData() const
{
    Q_D(const Request);
    return d->m_clientData;
}

void Request::setHandler(RequestHandler *handler)
{
    Q_D(Request);
    if (handler && d->m_handler) {
        qWarning() << "Request handler already set!";
        return;
    }
    d->m_handler = handler;
}

RequestHandler *Request::handler() const
{
    Q_D(const Request);
    return d->m_handler;
}

void Request::setCanceled()
{
    if (isInProgress()) {
        QVariantMap result;
        result[SSOUI_KEY_ERROR] = SignOn::QUERY_ERROR_CANCELED;

        setResult(result);
    }
}

#include "signonui-request.moc"
