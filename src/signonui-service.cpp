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

#include "debug.h"
#include "request.h"
#include "request-handler.h"
#include "request-manager.h"
#include "signonui-request.h"
#include "signonui-service.h"
#include "browser-request.h"

#include <QDBusArgument>
#include <QtQml>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

namespace SignOnUi {

static QVariant dbusValueToVariant(const QDBusArgument &argument)
{
    QVariant ret;

    /* Note: this function should operate recursively, but it doesn't. */
    if (argument.currentType() == QDBusArgument::MapType) {
        /* Assume that all maps are a{sv} */
        ret = qdbus_cast<QVariantMap>(argument);
    } else {
        /* We don't know how to handle other types */
        ret = argument.asVariant();
    }
    return ret;
}

static QVariantMap expandDBusArguments(const QVariantMap &dbusMap)
{
    QVariantMap map;
    QMapIterator<QString, QVariant> it(dbusMap);
    while (it.hasNext()) {
        it.next();
        if (qstrcmp(it.value().typeName(), "QDBusArgument") == 0) {
            QDBusArgument dbusValue = it.value().value<QDBusArgument>();
            map.insert(it.key(), dbusValueToVariant(dbusValue));
        } else {
            map.insert(it.key(), it.value());
        }
    }
    return map;
}

class ServicePrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(Service)

public:
    ServicePrivate(Service *service);
    ~ServicePrivate();

    void cancelUiRequest(const QString &requestId);
    void removeIdentityData(quint32 id);

private:
    mutable Service *q_ptr;
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

void ServicePrivate::cancelUiRequest(const QString &requestId)
{
    QVariantMap match;
    match.insert(SSOUI_KEY_REQUESTID, requestId);
    OnlineAccountsUi::Request *request =
        OnlineAccountsUi::Request::find(match);

    DEBUG() << "Cancelling request" << request;
    if (request != 0) {
        request->cancel();
    }
}

void ServicePrivate::removeIdentityData(quint32 id)
{
    /* Remove any data associated with the given identity. */

    /* The BrowserRequest class creates a directory to store the cookies */
    BrowserRequest::removeIdentityData(id);
}

Service::Service(QObject *parent):
    QObject(parent),
    d_ptr(new ServicePrivate(this))
{
    qmlRegisterType<SignOnUi::RequestHandler>("Ubuntu.OnlineAccounts.Plugin",
                                              1, 0, "RequestHandler");
}

Service::~Service()
{
}

QVariantMap Service::queryDialog(const QVariantMap &parameters)
{
    QVariantMap cleanParameters = expandDBusArguments(parameters);
    DEBUG() << "Got request:" << cleanParameters;

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);

    OnlineAccountsUi::Request *request =
        OnlineAccountsUi::Request::newRequest(connection(),
                                              message(),
                                              cleanParameters,
                                              this);

    /* Check if a RequestHandler has been setup to handle this request. If
     * so, bing the request object to the handler and start the request
     * immediately. */
    SignOnUi::Request *signonRequest =
        qobject_cast<SignOnUi::Request*>(request);
    Q_ASSERT(signonRequest != 0);

    RequestHandler *handler = RequestHandler::findMatching(cleanParameters);
    if (handler != 0) {
        DEBUG() << "Found RequestHandler!";
        signonRequest->setHandler(handler);
        QObject::connect(signonRequest, SIGNAL(completed()),
                         signonRequest, SLOT(deleteLater()));
        signonRequest->start();
    } else {
        // proceed normally
        OnlineAccountsUi::RequestManager::instance()->enqueue(request);
    }

    return QVariantMap();
}

QVariantMap Service::refreshDialog(const QVariantMap &newParameters)
{
    QVariantMap cleanParameters = expandDBusArguments(newParameters);
    QString requestId = Request::id(cleanParameters);
    // TODO find the request and update it

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);
    return QVariantMap();
}

void Service::cancelUiRequest(const QString &requestId)
{
    Q_D(Service);
    d->cancelUiRequest(requestId);
}

void Service::removeIdentityData(quint32 id)
{
    Q_D(Service);
    d->removeIdentityData(id);
}

#include "signonui-service.moc"
