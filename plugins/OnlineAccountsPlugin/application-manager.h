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

#ifndef OAU_APPLICATION_MANAGER_H
#define OAU_APPLICATION_MANAGER_H

#include "global.h"

#include <Accounts/Application>
#include <QObject>
#include <QStringList>
#include <QVariantMap>

class ApplicationManagerTest;

namespace OnlineAccountsUi {

class ApplicationManagerPrivate;
class OAP_EXPORT ApplicationManager: public QObject
{
    Q_OBJECT

public:
    static ApplicationManager *instance();

    Q_INVOKABLE QVariantMap applicationInfo(const QString &applicationId,
                                            const QString &profile);
    QVariantMap providerInfo(const QString &providerId) const;

    Q_INVOKABLE QStringList usefulProviders() const;

    Q_INVOKABLE QStringList addApplicationToAcl(const QStringList &acl,
                                                const QString &appId) const;
    Q_INVOKABLE QStringList removeApplicationFromAcl(const QStringList &acl,
                                                     const QString &appId) const;
    Accounts::Application applicationFromProfile(const QString &profile);

protected:
    explicit ApplicationManager(QObject *parent = 0);
    ~ApplicationManager();

private:
    static ApplicationManager *m_instance;
    ApplicationManagerPrivate *d_ptr;
    Q_DECLARE_PRIVATE(ApplicationManager)
    friend class ::ApplicationManagerTest;
};

} // namespace

#endif // OAU_APPLICATION_MANAGER_H
