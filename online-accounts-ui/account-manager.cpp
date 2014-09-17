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
#include "debug.h"

using namespace OnlineAccountsUi;
using namespace Accounts;

AccountManager *AccountManager::m_instance = 0;

AccountManager *AccountManager::instance()
{
    if (!m_instance) {
        m_instance = new AccountManager;
        /* to ensure that all the installed services are parsed into
         * libaccounts' DB, we enumerate them here.
         * TODO: a click package hook would be a more proper fix.
         */
        m_instance->serviceList();
    }

    return m_instance;
}

AccountManager::AccountManager(QObject *parent):
    Accounts::Manager(parent)
{
}

AccountManager::~AccountManager()
{
}
