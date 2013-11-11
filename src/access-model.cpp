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
#include "account-manager.h"
#include "debug.h"

#include <Accounts/Account>
#include <Accounts/Application>
#include <Accounts/Service>
#include <QSortFilterProxyModel>

using namespace OnlineAccountsUi;

namespace OnlineAccountsUi {

class AccessModelPrivate: public QSortFilterProxyModel
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(AccessModel)

public:
    inline AccessModelPrivate(AccessModel *accessModel);
    inline ~AccessModelPrivate();

    void ensureSupportedServices() const;

protected:
    bool filterAcceptsRow(int source_row,
                          const QModelIndex &source_parent) const Q_DECL_OVERRIDE;

private:
    mutable AccessModel *q_ptr;
    mutable Accounts::ServiceList m_supportedServices;
    QString m_lastItemText;
    QString m_applicationId;
};

} // namespace

AccessModelPrivate::AccessModelPrivate(AccessModel *accessModel):
    QSortFilterProxyModel(accessModel),
    q_ptr(accessModel)
{
}

AccessModelPrivate::~AccessModelPrivate()
{
}

void AccessModelPrivate::ensureSupportedServices() const
{
    if (!m_supportedServices.isEmpty()) return; // Nothing to do

    /* List all services supported by the accounts's provider. We know that the
     * account model is an instance of the accounts-qml-module's
     * AccountServiceModel, and that its "provider" property must be set. */
    QAbstractItemModel *accountModel = sourceModel();
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

bool AccessModelPrivate::filterAcceptsRow(int sourceRow,
                                          const QModelIndex &sourceParent) const
{
    Q_UNUSED(sourceParent);

    if (m_applicationId.isEmpty()) return true;

    /* We must avoid showing those accounts which have already been enabled for
     * this application. */
    ensureSupportedServices();
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
    Q_FOREACH(const Accounts::Service &service, m_supportedServices) {
        account->selectService(service);
        if (!account->isEnabled()) {
            allServicesEnabled = false;
            break;
        }
    }

    DEBUG() << account->id() << "allServicesEnabled" << allServicesEnabled;
    return !allServicesEnabled;
}

AccessModel::AccessModel(QObject *parent):
    QIdentityProxyModel(parent),
    d_ptr(new AccessModelPrivate(this))
{
    Q_D(AccessModel);

    QObject::connect(this, SIGNAL(rowsInserted(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
    QObject::connect(this, SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
                     this, SIGNAL(countChanged()));
    d->setDynamicSortFilter(true);
    setSourceModel(d);
}

AccessModel::~AccessModel()
{
}

void AccessModel::setAccountModel(QAbstractItemModel *accountModel)
{
    Q_D(AccessModel);

    d->setSourceModel(accountModel);
}

QAbstractItemModel *AccessModel::accountModel() const
{
    Q_D(const AccessModel);
    return d->sourceModel();
}

int AccessModel::rowCount(const QModelIndex &parent) const
{
    return QIdentityProxyModel::rowCount(parent) + 1;
}

void AccessModel::setLastItemText(const QString &text)
{
    Q_D(AccessModel);

    if (text == d->m_lastItemText) return;
    d->m_lastItemText = text;
    Q_EMIT lastItemTextChanged();

    // TODO: emit dataChanged
}

QString AccessModel::lastItemText() const
{
    Q_D(const AccessModel);
    return d->m_lastItemText;
}

void AccessModel::setApplicationId(const QString &applicationId)
{
    Q_D(AccessModel);

    if (applicationId == d->m_applicationId) return;
    d->m_applicationId = applicationId;
    Q_EMIT applicationIdChanged();

    d->m_supportedServices.clear();
    /* Trigger a refresh of the filtered model */
    d->invalidateFilter();
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

QVariant AccessModel::data(const QModelIndex &index, int role) const
{
    Q_D(const AccessModel);
    if (index.row() < d->rowCount()) {
        return QIdentityProxyModel::data(index, role);
    } else {
        return d->m_lastItemText;
    }
}

QHash<int, QByteArray> AccessModel::roleNames() const
{
    Q_D(const AccessModel);
    return d->roleNames();
}

QModelIndex AccessModel::index(int row, int column,
                               const QModelIndex &parent) const
{
    Q_D(const AccessModel);
    if (row < d->rowCount()) {
        return QIdentityProxyModel::index(row, column, parent);
    } else {
        return createIndex(row, column);
    }
}

#include "access-model.moc"
