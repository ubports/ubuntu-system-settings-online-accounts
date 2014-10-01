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

#ifndef OAU_ACCOUNT_MANAGER_H
#define OAU_ACCOUNT_MANAGER_H

#include <Accounts/Manager>

namespace OnlineAccountsUi {

class AccountManager: public Accounts::Manager
{
    Q_OBJECT

public:
    static AccountManager *instance();

protected:
    explicit AccountManager(QObject *parent = 0);
    ~AccountManager();

private:
    static AccountManager *m_instance;
};

} // namespace

#endif // OAU_ACCOUNT_MANAGER_H
