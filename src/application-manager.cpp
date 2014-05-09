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
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

using namespace OnlineAccountsUi;

ApplicationManager *ApplicationManager::m_instance = 0;

namespace OnlineAccountsUi {
class ApplicationManagerPrivate
{
public:
    ApplicationManagerPrivate();

    QString applicationProfile(const QString &applicationId) const;
    bool applicationMatchesProfile(const Accounts::Application &application,
                                   const QString &profile) const;
    static QString stripVersion(const QString &appId);
};
} // namespace

ApplicationManagerPrivate::ApplicationManagerPrivate()
{
}

QString ApplicationManagerPrivate::applicationProfile(const QString &applicationId) const
{
    /* We need to load the XML file and look for the "profile" element. The
     * file lookup would become unnecessary if a domDocument() method were
     * added to the Accounts::Application class. */
    QString localShare =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QFile file(QString("%1/accounts/applications/%2.application").
               arg(localShare).arg(applicationId));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        DEBUG() << "file not found:" << file.fileName();
        /* libaccounts would fall back to looking into /usr/share/accounts/,
         * but we know that .click packages don't install files in there, and
         * currently the profile information is only attached to click
         * applications. Therefore, if we don't find the file in
         * ~/.local/share/accounts/, we can assume we won't find the profile
         * info anywhere.
         */
        return QString();
    }
    QDomDocument doc;
    doc.setContent(&file);
    const QDomElement root = doc.documentElement();
    return root.firstChildElement(QStringLiteral("profile")).text();
}

bool ApplicationManagerPrivate::applicationMatchesProfile(const Accounts::Application &application,
                                                          const QString &profile) const
{
    /* We don't restrict unconfined apps. */
    if (profile == QStringLiteral("unconfined")) return true;

    /* It's a confined app. We must make sure that the applicationId it
     * specified matches the apparmor profile.
     */
    QString declaredProfile = applicationProfile(application.name());
    return declaredProfile == profile;
}

QString ApplicationManagerPrivate::stripVersion(const QString &appId)
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

QVariantMap ApplicationManager::applicationInfo(const QString &claimedAppId,
                                                const QString &profile)
{
    Q_D(const ApplicationManager);

    if (Q_UNLIKELY(profile.isEmpty())) return QVariantMap();

    QString applicationId = claimedAppId;
    if (profile.startsWith(applicationId)) {
        /* Click packages might declare just the package name as application
         * ID, but in order to find the correct application file we need the
         * complete ID (with application title and version. */
        applicationId = profile;
    }

    Accounts::Application application =
        AccountManager::instance()->application(applicationId);

    /* Make sure that the app is who it claims to be */
    if (!d->applicationMatchesProfile(application, profile)) {
        DEBUG() << "Given applicationId doesn't match profile";
        return QVariantMap();
    }

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

QVariantMap ApplicationManager::providerInfo(const QString &providerId) const
{
    Accounts::Provider provider =
        AccountManager::instance()->provider(providerId);

    QVariantMap info;
    info.insert(QStringLiteral("id"), providerId);
    info.insert(QStringLiteral("displayName"), provider.displayName());
    info.insert(QStringLiteral("icon"), provider.iconName());
    return info;
}

QStringList
ApplicationManager::addApplicationToAcl(const QStringList &acl,
                                        const QString &applicationId) const
{
    Q_D(const ApplicationManager);

    QStringList newAcl = acl;
    QString profile = d->applicationProfile(applicationId);
    DEBUG() << "profile of" << applicationId << ":" << profile;
    if (!profile.isEmpty()) {
        newAcl.append(profile);
    }
    return newAcl;
}

QStringList
ApplicationManager::removeApplicationFromAcl(const QStringList &acl,
                                             const QString &applicationId) const
{
    Q_D(const ApplicationManager);

    QString profile = d->applicationProfile(applicationId);
    if (profile.isEmpty()) {
        return acl;
    }

    QStringList newAcl;
    QString unversionedProfile =
        ApplicationManagerPrivate::stripVersion(profile);
    Q_FOREACH(const QString &app, acl) {
        if (app != profile &&
            (unversionedProfile.isEmpty() ||
             !acl.startsWith(unversionedProfile))) {
            newAcl.append(app);
        }
    }
    return newAcl;
}
