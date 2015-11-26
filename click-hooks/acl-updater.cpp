/*
 * Copyright (C) 2015 Canonical Ltd.
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

#include "acl-updater.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QObject>
#include <SignOn/Identity>
#include <SignOn/IdentityInfo>

class AclUpdaterPrivate: public QObject
{
    Q_OBJECT

public:
    AclUpdaterPrivate();

public Q_SLOTS:
    void onInfo(const SignOn::IdentityInfo &info);
    void onStored(const quint32 id);
    void onError(const SignOn::Error &error);

private:
    friend class AclUpdater;
    QEventLoop m_loop;
};

AclUpdaterPrivate::AclUpdaterPrivate():
    QObject()
{
}

void AclUpdaterPrivate::onInfo(const SignOn::IdentityInfo &info)
{
    SignOn::Identity *identity = qobject_cast<SignOn::Identity*>(sender());

    QString shortAppId = identity->property("appToBeRemoved").toString();
    QStringList acl;
    Q_FOREACH(const QString &token, info.accessControlList()) {
        if (!token.startsWith(shortAppId)) {
            acl.append(token);
        }
    }

    if (acl != info.accessControlList()) {
        SignOn::IdentityInfo newInfo(info);
        newInfo.setAccessControlList(acl);
        identity->storeCredentials(newInfo);
    } else {
        qDebug() << shortAppId << "was not in ACL of" << info.id();
        m_loop.exit(0);
    }
}

void AclUpdaterPrivate::onStored(const quint32 id)
{
    Q_UNUSED(id);
    m_loop.exit(0);
}

void AclUpdaterPrivate::onError(const SignOn::Error &err)
{
    qWarning() << "Error occurred updating ACL" << err.message();
    m_loop.exit(1);
}

AclUpdater::AclUpdater():
    d_ptr(new AclUpdaterPrivate)
{
}

AclUpdater::~AclUpdater()
{
    delete d_ptr;
}

bool AclUpdater::removeApp(const QString &shortAppId, uint credentialsId)
{
    Q_D(AclUpdater);

    if (credentialsId == 0 ||
        !shortAppId.contains('_')) return false;

    SignOn::Identity *identity =
        SignOn::Identity::existingIdentity(credentialsId, d);
    if (Q_UNLIKELY(!identity)) return false;

    identity->setProperty("appToBeRemoved", shortAppId);
    QObject::connect(identity, SIGNAL(info(const SignOn::IdentityInfo&)),
                     d, SLOT(onInfo(const SignOn::IdentityInfo&)));
    QObject::connect(identity, SIGNAL(credentialsStored(const quint32)),
                     d, SLOT(onStored(const quint32)));
    QObject::connect(identity, SIGNAL(error(const SignOn::Error &)),
                     d, SLOT(onError(const SignOn::Error &)));
    identity->queryInfo();

    bool ok = (d->m_loop.exec() == 0);
    delete identity;
    return ok;
}

#include "acl-updater.moc"
