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

#ifndef ACCOUNTS_HOOK_ACL_UPDATER
#define ACCOUNTS_HOOK_ACL_UPDATER

#include <QString>

class AclUpdaterPrivate;
class AclUpdater
{
public:
    AclUpdater();
    virtual ~AclUpdater();

    bool removeApp(const QString &shortAppId, uint credentialsId);

private:
    Q_DECLARE_PRIVATE(AclUpdater);
    AclUpdaterPrivate *d_ptr;
};

#endif // ACCOUNTS_HOOK_ACL_UPDATER
