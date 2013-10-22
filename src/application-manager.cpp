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

#include "account-manager.h"
#include "application-manager.h"
#include "debug.h"

#include <Accounts/Application>

using namespace OnlineAccountsUi;

ApplicationManager *ApplicationManager::m_instance = 0;

namespace OnlineAccountsUi {
class ApplicationManagerPrivate
{
public:
    ApplicationManagerPrivate();
};
} // namespace

ApplicationManagerPrivate::ApplicationManagerPrivate()
{
}

ApplicationManager *ApplicationManager::instance()
{
    if (!m_instance) {
        m_instance = new ApplicationManager;
    }

    return m_instance;
}

ApplicationManager::ApplicationManager(QObject *parent):
    QObject(parent),
    d_ptr(new ApplicationManagerPrivate)
{
}

ApplicationManager::~ApplicationManager()
{
    delete d_ptr;
}

QVariantMap ApplicationManager::applicationInfo(const QString &applicationId,
                                                const QString &profile)
{
    if (Q_UNLIKELY(profile.isEmpty())) return QVariantMap();

    Accounts::Application application =
        AccountManager::instance()->application(applicationId);

    /* TODO: load the .desktop file and check if it matches the apparmor
     * profile */

    QVariantMap app;
    app.insert(QStringLiteral("id"), applicationId);
    app.insert(QStringLiteral("displayName"), application.displayName());
    app.insert(QStringLiteral("icon"), application.iconName());
    app.insert(QStringLiteral("profile"), profile);

    /* List all the services supported by this application */
    QVariantList serviceIds;
    Accounts::ServiceList allServices =
        AccountManager::instance()->serviceList();
    Q_FOREACH(const Accounts::Service &service, allServices) {
        if (!application.serviceUsage(service).isEmpty()) {
            serviceIds.append(service.name());
        }
    }
    app.insert(QStringLiteral("services"), serviceIds);

    return app;
}
