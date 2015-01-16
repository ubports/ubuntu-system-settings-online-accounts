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
#include "request-manager.h"
#include "signonui-service.h"

#include <QDBusArgument>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QNetworkCookie>
#include <QStandardPaths>
#include <QVariant>
#include <SignOn/uisessiondata_priv.h>

using namespace SignOnUi;

namespace SignOnUi {

static QList<QByteArray> cookiesFromVariant(const QVariantList &cl)
{
    QList<QByteArray> cookies;
    Q_FOREACH(QVariant cookie, cl) {
        if (!cookie.canConvert(QVariant::Map)) {
            continue;
        }

        QNetworkCookie nc;
        QVariantMap vm = cookie.toMap();
        if (!vm.contains("name") || !vm.contains("value")) {
            continue;
        }

        nc.setName(vm.value("name").toByteArray());
        nc.setValue(vm.value("value").toByteArray());
        nc.setDomain(vm.value("domain").toString());
        nc.setPath(vm.value("path").toString());
        if (vm.contains("httponly") &&
            vm.value("httponly").canConvert(QVariant::Bool)) {
            nc.setHttpOnly(vm.value("httponly").toBool());
        }

        if (vm.contains("issecure") &&
            vm.value("issecure").canConvert(QVariant::Bool)) {
            nc.setSecure(vm.value("issecure").toBool());
        }

        if (vm.contains("expirationdate") &&
            vm.value("expirationdate").canConvert(QMetaType::QDateTime)) {
            nc.setExpirationDate(vm.value("expirationdate").toDateTime());
        }

        cookies.append(nc.toRawForm());
    }
    return cookies;
}

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
    RawCookies cookiesForIdentity(quint32 id, qint64 &timestamp) const;

    static QString rootDirForIdentity(quint32 id);

private:
    mutable Service *q_ptr;
};

} // namespace

ServicePrivate::ServicePrivate(Service *service):
    QObject(service),
    q_ptr(service)
{
    qRegisterMetaType<RawCookies>("RawCookies");
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

QString ServicePrivate::rootDirForIdentity(quint32 id)
{
    /* the BrowserRequest class instructs the webview to store its cookies and
     * local data into
     * ~/.cache/online-accounts-ui/id-<signon-id>-<provider-id>/; while we
     * don't normally expect to find more than one entry for a given signon-id,
     * it's a possibility we cannot completely discard, since in older versions
     * we were not appending the "-<provider-id>" suffix. Besides, unless we
     * look up into the accounts DB, we don't know the value for the provider
     * ID.
     * Because of both of these reasons, we list all the directories whose name
     * starts with "id-<signon-id>" and pick the most recent one.
     */
    QString cachePath =
        QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    QDir cacheDir(cachePath + QStringLiteral("/online-accounts-ui"));
    QString nameFilter = QString("id-%1*").arg(id);
    QStringList rootDirs = cacheDir.entryList(QStringList() << nameFilter,
                                              QDir::Dirs, QDir::Time);
    return rootDirs.count() > 0 ? cacheDir.filePath(rootDirs.at(0)) : QString();
}

void ServicePrivate::removeIdentityData(quint32 id)
{
    /* Remove any data associated with the given identity. */
    QString rootDirName = rootDirForIdentity(id);
    if (rootDirName.isEmpty()) return;
    QDir rootDir(rootDirName);
    rootDir.removeRecursively();
}

RawCookies ServicePrivate::cookiesForIdentity(quint32 id,
                                              qint64 &timestamp) const
{
    RawCookies cookies;

    QFileInfo fileInfo(rootDirForIdentity(id) + "/cookies.json");
    if (!fileInfo.exists()) {
        DEBUG() << "File does not exist:" << fileInfo.filePath();
        return cookies;
    }
    timestamp = fileInfo.lastModified().toMSecsSinceEpoch() / 1000;

    QFile file(fileInfo.filePath());
    if (Q_UNLIKELY(!file.open(QIODevice::ReadOnly | QIODevice::Text))) {
        qWarning() << "Cannot open file" << fileInfo.filePath();
        return cookies;
    }

    QByteArray contents = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(contents);
    if (doc.isEmpty() || !doc.isArray()) return cookies;

    QVariantList cookieVariants = doc.array().toVariantList();
    return cookiesFromVariant(cookieVariants);
}

Service::Service(QObject *parent):
    QObject(parent),
    d_ptr(new ServicePrivate(this))
{
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
        new OnlineAccountsUi::Request(connection(),
                                      message(),
                                      cleanParameters,
                                      this);

    OnlineAccountsUi::RequestManager::instance()->enqueue(request);

    return QVariantMap();
}

QVariantMap Service::refreshDialog(const QVariantMap &newParameters)
{
    QVariantMap cleanParameters = expandDBusArguments(newParameters);
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

void Service::cookiesForIdentity(quint32 id,
                                 RawCookies &cookies, qint64 &timestamp)
{
    Q_D(Service);
    cookies = d->cookiesForIdentity(id, timestamp);
}

#include "signonui-service.moc"
