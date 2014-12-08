/*
 * Copyright (C) 2014 Canonical Ltd.
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
#include "libaccounts-service.h"
#include "utils.h"

#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QVariantMap>

using namespace OnlineAccountsUi;

static QString stripVersion(const QString &appId)
{
    QStringList components = appId.split('_');
    if (components.count() != 3) return QString();

    /* Click packages have a profile of the form
     *  $name_$application_$version
     * (see https://wiki.ubuntu.com/SecurityTeam/Specifications/ApplicationConfinement/Manifest#Click)
     *
     * We assume that this is a click package, and strip out the last part.
     */
    components.removeLast();
    return components.join('_');
}

namespace OnlineAccountsUi {

struct ServiceChanges {
    QString service;
    QString serviceType;
    quint32 serviceId;
    QVariantMap settings;
    QStringList removedKeys;
};

struct AccountChanges {
    quint32 accountId;
    bool created;
    bool deleted;
    QString provider;
    QList<ServiceChanges> serviceChanges;
};

class LibaccountsServicePrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(LibaccountsService)

public:
    LibaccountsServicePrivate(LibaccountsService *q);
    ~LibaccountsServicePrivate() {};

    void writeChanges(const AccountChanges &changes);

private Q_SLOTS:
    void onAccountSynced();
    void onAccountError(Accounts::Error error);

private:
    Accounts::Manager m_manager;
    QHash<Accounts::Account *,QDBusMessage> m_pendingWrites;
    mutable LibaccountsService *q_ptr;
};

} // namespace

LibaccountsServicePrivate::LibaccountsServicePrivate(LibaccountsService *q):
    QObject(q),
    m_manager(new Accounts::Manager(this)),
    q_ptr(q)
{
}

void LibaccountsServicePrivate::writeChanges(const AccountChanges &changes)
{
    Q_Q(LibaccountsService);

    Accounts::Account *account;

    if (changes.created) {
        account = m_manager.createAccount(changes.provider);
    } else {
        account = m_manager.account(changes.accountId);
        if (Q_UNLIKELY(!account)) {
            qWarning() << "Couldn't load account" << changes.accountId;
            return;
        }
    }

    Q_ASSERT(account);

    if (changes.deleted) {
        account->remove();
    } else {
        Q_FOREACH(const ServiceChanges &sc, changes.serviceChanges) {
            Accounts::Service service = m_manager.service(sc.service);
            if (Q_UNLIKELY(!service.isValid())) {
                qWarning() << "Invalid service" << sc.service;
                continue;
            }

            account->selectService(service);

            QMapIterator<QString, QVariant> it(sc.settings);
            while (it.hasNext()) {
                it.next();
                account->setValue(it.key(), it.value());
            }

            Q_FOREACH(const QString &key, sc.removedKeys) {
                account->remove(key);
            }
        }
    }

    m_pendingWrites.insert(account, q->message());
    QObject::connect(account, SIGNAL(synced()),
                     this, SLOT(onAccountSynced()));
    QObject::connect(account, SIGNAL(Accounts::Error),
                     this, SLOT(onAccountError(Accounts::Error)));
    account->sync();
}

void LibaccountsServicePrivate::onAccountSynced()
{
    Q_Q(LibaccountsService);

    Accounts::Account *account = qobject_cast<Accounts::Account*>(sender());
    account->deleteLater();

    QDBusMessage message = m_pendingWrites.take(account);
    q->connection().send(message.createReply());
}

void LibaccountsServicePrivate::onAccountError(Accounts::Error error)
{
    Q_Q(LibaccountsService);

    Accounts::Account *account = qobject_cast<Accounts::Account*>(sender());
    account->deleteLater();

    QDBusMessage message = m_pendingWrites.take(account);
    QDBusMessage reply =
        message.createErrorReply(QDBusError::InternalError, error.message());
    q->connection().send(reply);
}

LibaccountsService::LibaccountsService(QObject *parent):
    QObject(parent),
    d_ptr(new LibaccountsServicePrivate(this))
{
}

LibaccountsService::~LibaccountsService()
{
    delete d_ptr;
}

void LibaccountsService::store(const QDBusMessage &msg)
{
    Q_D(LibaccountsService);

    DEBUG() << "Got request:" << msg;

    /* The following line tells QtDBus not to generate a reply now */
    setDelayedReply(true);

    AccountChanges changes;

    // signature: "ubbsa(ssua{sv}as)"
    QList<QVariant> args = msg.arguments();
    int n = 0;
    changes.accountId = args.value(n++).toUInt();
    changes.created = args.value(n++).toBool();
    changes.deleted = args.value(n++).toBool();
    changes.provider = args.value(n++).toString();

    /* before continuing demarshalling the arguments, check if the provider ID
     * matches the apparmor label of the peer; if it doesn't, we shouldn't
     * honour this request. */
    QString profile = apparmorProfileOfPeer(msg);
    if (stripVersion(profile) != changes.provider) {
        DEBUG() << "Declining AccountManager store request to" << profile <<
            "for provider" << changes.provider;
        QDBusMessage reply = msg.createErrorReply(QDBusError::AccessDenied,
                                                  "Profile/provider mismatch");
        connection().send(reply);
        return;
    }

    QDBusArgument dbusChanges = args.value(n++).value<QDBusArgument>();
    dbusChanges.beginArray();
    while (!dbusChanges.atEnd()) {
        ServiceChanges sc;
        dbusChanges.beginStructure();
        dbusChanges >> sc.service;
        dbusChanges >> sc.serviceType;
        dbusChanges >> sc.serviceId;
        dbusChanges >> sc.settings;
        dbusChanges >> sc.removedKeys;
        dbusChanges.endStructure();

        changes.serviceChanges.append(sc);
    }
    dbusChanges.endArray();

    d->writeChanges(changes);
}

#include "libaccounts-service.moc"
