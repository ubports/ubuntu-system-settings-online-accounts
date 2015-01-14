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

#include "signonui-request.h"

#include "browser-request.h"
#include "debug.h"
#include "dialog-request.h"
#include "globals.h"

#include <Accounts/Account>
#include <OnlineAccountsPlugin/account-manager.h>
#include <OnlineAccountsPlugin/request-handler.h>
#include <QDBusArgument>
#include <QPointer>
#include <SignOn/uisessiondata.h>
#include <SignOn/uisessiondata_priv.h>
#include <sys/apparmor.h>

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
    Accounts::Account *findAccount();

private:
    mutable Request *q_ptr;
    QVariantMap m_clientData;
    QPointer<RequestHandler> m_handler;
    Accounts::Account *m_account;
};

} // namespace

RequestPrivate::RequestPrivate(Request *request):
    QObject(request),
    q_ptr(request),
    m_handler(0)
{
    const QVariantMap &parameters = request->parameters();
    if (parameters.contains(SSOUI_KEY_CLIENT_DATA)) {
        QVariant variant = parameters[SSOUI_KEY_CLIENT_DATA];
        m_clientData = (variant.type() == QVariant::Map) ?
            variant.toMap() :
            qdbus_cast<QVariantMap>(variant.value<QDBusArgument>());
    }

    m_account = findAccount();
}

RequestPrivate::~RequestPrivate()
{
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
    OnlineAccountsUi::AccountManager *manager =
        OnlineAccountsUi::AccountManager::instance();
    Q_FOREACH(Accounts::AccountId accountId, manager->accountList()) {
        Accounts::Account *account = manager->account(accountId);
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

#ifndef NO_REQUEST_FACTORY
Request *Request::newRequest(int id,
                             const QString &clientProfile,
                             const QVariantMap &parameters,
                             QObject *parent)
{
    if (parameters.contains(SSOUI_KEY_OPENURL)) {
        return new SignOnUi::BrowserRequest(id, clientProfile,
                                            parameters, parent);
    } else {
        return new SignOnUi::DialogRequest(id, clientProfile,
                                           parameters, parent);
    }
}
#endif

static QString findClientProfile(const QString &clientProfile,
                                 const QVariantMap &parameters)
{
    QString profile = clientProfile;
    /* If the request is coming on the SignOnUi interface from an
     * unconfined process and it carries the SSOUI_KEY_PID key, it means that
     * it's coming from signond. In that case, we want to know what was the
     * client which originated the call.
     */
    if (profile == "unconfined" &&
        parameters.contains(SSOUI_KEY_PID)) {
        pid_t pid = parameters.value(SSOUI_KEY_PID).toUInt();
        char *con = NULL, *mode = NULL;
        int ret = aa_gettaskcon(pid, &con, &mode);
        if (Q_LIKELY(ret >= 0)) {
            profile = QString::fromUtf8(con);
            /* libapparmor allocates "con" and "mode" in a single allocation,
             * so freeing "con" is actually freeing both. */
            free(con);
        } else {
            qWarning() << "Couldn't get apparmor profile of PID" << pid;
        }
    }
    return profile;
}

Request::Request(int id,
                 const QString &clientProfile,
                 const QVariantMap &parameters,
                 QObject *parent):
    OnlineAccountsUi::Request(SIGNONUI_INTERFACE, id,
                              findClientProfile(clientProfile, parameters),
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
    Q_D(Request);

    /* Show the window only if we are in a prompt session */
    if (qgetenv("MIR_SOCKET").isEmpty()) {
        QVariantMap result;
        result[SSOUI_KEY_ERROR] = SignOn::QUERY_ERROR_FORBIDDEN;
        setResult(result);
    } else {
        OnlineAccountsUi::Request::setWindow(window);
    }
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
    return d->m_account ? d->m_account->providerName() :
        d->m_clientData.value("providerId").toString();
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
    QVariantMap result;
    result[SSOUI_KEY_ERROR] = SignOn::QUERY_ERROR_CANCELED;

    setResult(result);
}

#include "signonui-request.moc"
