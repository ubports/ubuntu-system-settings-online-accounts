/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This file is part of OnlineAccountsClient.
 *
 * OnlineAccountsClient is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * OnlineAccountsClient is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OnlineAccountsClient.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef ONLINE_ACCOUNTS_CLIENT_SETUP_H
#define ONLINE_ACCOUNTS_CLIENT_SETUP_H

#include <QObject>

namespace OnlineAccountsClient {

class SetupPrivate;
class Setup: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serviceTypeId READ serviceTypeId \
               WRITE setServiceTypeId NOTIFY serviceTypeIdChanged)
    Q_PROPERTY(QString providerId READ providerId \
               WRITE setProviderId NOTIFY providerIdChanged)

public:
    Setup(QObject *parent = 0);
    ~Setup();

    void setServiceTypeId(const QString &serviceTypeId);
    QString serviceTypeId() const;

    void setProviderId(const QString &providerId);
    QString providerId() const;

    Q_INVOKABLE void exec();

Q_SIGNALS:
    void serviceTypeIdChanged();
    void providerIdChanged();
    void finished();

private:
    SetupPrivate *d_ptr;
    Q_DECLARE_PRIVATE(Setup)
};

}; // namespace

#endif // ONLINE_ACCOUNTS_CLIENT_SETUP_H
