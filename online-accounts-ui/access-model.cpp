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

#include "access-model.h"
#include "debug.h"

#include <Accounts/Account>
#include <Accounts/Application>
#include <Accounts/Service>
#include <OnlineAccountsPlugin/account-manager.h>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

class AccessModelPrivate
{
    Q_DECLARE_PUBLIC(AccessModel)

public:
    inline AccessModelPrivate(AccessModel *accessModel);
    inline ~AccessModelPrivate();

    void ensureSupportedServices() const;

private:
    mutable AccessModel *q_ptr;
    mutable Accounts::ServiceList m_supportedServices;
    QString m_lastItemText;
    QString m_applicationId;
};

} // namespace

AccessModelPrivate::AccessModelPrivate(AccessModel *accessModel):
    q_ptr(accessModel)
{
}

AccessModelPrivate::~AccessModelPrivate()
{
}

void AccessModelPrivate::ensureSupportedServices() const
{
    Q_Q(const AccessModel);

    if (!m_supportedServices.isEmpty()) return; // Nothing to do

    /* List all services supported by the accounts's provider. We know that the
     * account model is an instance of the accounts-qml-module's
     * AccountServiceModel, and that its "provider" property must be set. */
    QAbstractItemModel *accountModel = q->sourceModel();
    if (!accountModel) return;

    QString providerId = accountModel->property("provider").toString();
    if (providerId.isEmpty()) return;

    AccountManager *manager = AccountManager::instance();
    Accounts::Application application = manager->application(m_applicationId);
    Accounts::ServiceList allServices = manager->serviceList();
    Q_FOREACH(const Accounts::Service &service, allServices) {
        if (service.provider() == providerId &&
            !application.serviceUsage(service).isEmpty()) {
            m_supportedServices.append(service);
        }
    }
}

AccessModel::AccessModel(QObject *parent):
    QSortFilterProxyModel(parent),
    d_ptr(new AccessModelPrivate(this))
{
    QObject::connect(this, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
    QObject::connect(this, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
    setDynamicSortFilter(true);
}

AccessModel::~AccessModel()
{
}

void AccessModel::setAccountModel(QAbstractItemModel *accountModel)
{
    setSourceModel(accountModel);
    Q_EMIT accountModelChanged();
}

QAbstractItemModel *AccessModel::accountModel() const
{
    return sourceModel();
}

void AccessModel::setApplicationId(const QString &applicationId)
{
    Q_D(AccessModel);

    if (applicationId == d->m_applicationId) return;
    d->m_applicationId = applicationId;
    Q_EMIT applicationIdChanged();

    d->m_supportedServices.clear();
    /* Trigger a refresh of the filtered model */
    invalidateFilter();
}

QString AccessModel::applicationId() const
{
    Q_D(const AccessModel);
    return d->m_applicationId;
}

QVariant AccessModel::get(int row, const QString &roleName) const
{
    int role = roleNames().key(roleName.toLatin1(), -1);
    return data(index(row, 0), role);
}

bool AccessModel::filterAcceptsRow(int sourceRow,
                                   const QModelIndex &sourceParent) const
{
    Q_D(const AccessModel);

    Q_UNUSED(sourceParent);

    if (d->m_applicationId.isEmpty()) return true;

    /* We must avoid showing those accounts which have already been enabled for
     * this application. */
    d->ensureSupportedServices();
    QVariant result;
    bool ok = QMetaObject::invokeMethod(sourceModel(), "get",
                                        Qt::DirectConnection,
                                        Q_RETURN_ARG(QVariant, result),
                                        Q_ARG(int, sourceRow),
                                        Q_ARG(QString, "accountHandle"));
    if (Q_UNLIKELY(!ok)) return false;

    QObject *accountHandle = result.value<QObject*>();
    Accounts::Account *account =
        qobject_cast<Accounts::Account*>(accountHandle);
    if (Q_UNLIKELY(!account)) return false;

    bool allServicesEnabled = true;
    Q_FOREACH(const Accounts::Service &service, d->m_supportedServices) {
        account->selectService(service);
        if (!account->isEnabled()) {
            allServicesEnabled = false;
            break;
        }
    }

    DEBUG() << account->id() << "allServicesEnabled" << allServicesEnabled;
    return !allServicesEnabled;
}
