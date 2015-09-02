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

#include "application-manager-mock.h"

#include <QDebug>

using namespace OnlineAccountsUi;

ApplicationManager *ApplicationManager::m_instance = 0;

ApplicationManagerPrivate::ApplicationManagerPrivate(ApplicationManager *q):
    QObject(q),
    q_ptr(q)
{
}

ApplicationManagerPrivate::~ApplicationManagerPrivate()
{
}

ApplicationManager::ApplicationManager(QObject *parent):
    QObject(parent),
    d_ptr(new ApplicationManagerPrivate(this))
{
    m_instance = this;
}

ApplicationManager::~ApplicationManager()
{
    m_instance = 0;
}

ApplicationManager *ApplicationManager::instance()
{
    if (!m_instance) {
        new ApplicationManager;
    }
    return m_instance;
}

QVariantMap ApplicationManager::applicationInfo(const QString &applicationId,
                                             const QString &profile)
{
    Q_D(ApplicationManager);
    Q_EMIT d->applicationInfoCalled(applicationId, profile);
    return d->m_applicationInfo[applicationId];
}

QVariantMap ApplicationManager::providerInfo(const QString &providerId) const
{
    Q_D(const ApplicationManager);
    return d->m_providerInfo[providerId];
}
