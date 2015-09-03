/*
 * This file is part of libOnlineAccounts
 *
 * Copyright (C) 2015 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OAD_FAKE_SIGNOND_H
#define OAD_FAKE_SIGNOND_H

#include <QVariantMap>
#include <libqtdbusmock/DBusMock.h>

class FakeSignond
{
public:
    FakeSignond(QtDBusMock::DBusMock *mock): m_mock(mock) {
        m_mock->registerTemplate("com.google.code.AccountsSSO.SingleSignOn",
                                 SIGNOND_MOCK_TEMPLATE,
                                 QDBusConnection::SessionBus);
    }

    void addIdentity(uint id, const QVariantMap &info) {
        mockedAuthService().call("AddIdentity", id, info);
    }

private:
    OrgFreedesktopDBusMockInterface &mockedAuthService() {
        return m_mock->mockInterface("com.google.code.AccountsSSO.SingleSignOn",
                                     "/com/google/code/AccountsSSO/SingleSignOn",
                                     "com.google.code.AccountsSSO.SingleSignOn.AuthService",
                                     QDBusConnection::SessionBus);
    }

private:
    QtDBusMock::DBusMock *m_mock;
};

#endif // OAD_FAKE_SIGNOND_H
