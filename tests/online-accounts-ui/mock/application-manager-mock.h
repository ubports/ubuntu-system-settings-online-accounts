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

#ifndef MOCK_APPLICATION_MANAGER_H
#define MOCK_APPLICATION_MANAGER_H

#include <OnlineAccountsPlugin/application-manager.h>

#include <QHash>
#include <QObject>

namespace OnlineAccountsUi {

class ApplicationManagerPrivate: public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(ApplicationManager)

public:
    ApplicationManagerPrivate(ApplicationManager *q);
    ~ApplicationManagerPrivate();
    static ApplicationManagerPrivate *mocked(ApplicationManager *r) { return r->d_ptr; }

    void setApplicationInfo(const QString &applicationId,
                            const QVariantMap &info) {
        m_applicationInfo[applicationId] = info;
    }

    void setProviderInfo(const QString &providerId, const QVariantMap &info) {
        m_providerInfo[providerId] = info;
    }

Q_SIGNALS:
    void applicationInfoCalled(QString applicationId, QString profile);

public:
    QHash<QString,QVariantMap> m_applicationInfo;
    QHash<QString,QVariantMap> m_providerInfo;
    mutable ApplicationManager *q_ptr;
};

} // namespace

#endif // MOCK_APPLICATION_MANAGER_H
