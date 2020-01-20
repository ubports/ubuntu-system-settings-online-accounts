/*
 * Copyright (C) 2013-2015 Canonical Ltd.
 *
 * Contact: Alberto Mardegan <alberto.mardegan@canonical.com>
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

import QtQuick 2.9
import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3 as ListItem
import Ubuntu.OnlineAccounts 0.1

Column {
    id: root

    property variant account

    anchors.left: parent.left
    anchors.right: parent.right

    ListItem.Standard {
        text: i18n.dtr("ubuntu-system-settings-online-accounts", "Access to this account:")
    }

    AccountServiceModel {
        id: accountServices
        includeDisabled: true
        account: root.account.objectHandle
    }

    AccountService {
        id: globalAccountService
        objectHandle: root.account.accountServiceHandle
    }

    Credentials {
        id: credentials
        credentialsId: globalAccountService.authData.credentialsId
    }

    Repeater {
        model: accountServices
        delegate: ServiceItem {
            accountServiceHandle: model.accountServiceHandle
            onApplicationAdded: {
                var newAcl = ApplicationManager.addApplicationToAcl(credentials.acl,
                                                                    applicationId)
                if (newAcl != credentials.acl) {
                    credentials.acl = newAcl
                    credentials.sync()
                }
            }

            onApplicationRemoved: {
                var newAcl =
                    ApplicationManager.removeApplicationFromAcl(credentials.acl,
                                                                applicationId)
                if (newAcl != credentials.acl) {
                    credentials.acl = newAcl
                    credentials.sync()
                }
            }
        }
    }
}
