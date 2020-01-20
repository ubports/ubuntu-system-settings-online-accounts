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
import Ubuntu.Components.Popups 1.3
import Ubuntu.OnlineAccounts 0.1

Column {
    id: root

    property variant __account: account

    signal finished

    anchors.left: parent.left
    anchors.right: parent.right

    ListItem.SingleValue {
        text: i18n.dtr("ubuntu-system-settings-online-accounts", "ID")
        value: account.displayName
    }

    ServiceSwitches {
        account: __account
        enabled: __account.enabled
        opacity: enabled ? 1 : 0.5
    }

    ListItem.SingleControl {
        control: Button {
            text: i18n.dtr("ubuntu-system-settings-online-accounts", "Remove account…")
            width: parent.width - units.gu(4)
            onClicked: PopupUtils.open(removalConfirmationComponent)
        }
        showDivider: false
    }

    Component {
        id: removalConfirmationComponent
        RemovalConfirmation {
            accountName: __account.provider.displayName
            onClosed: {
                if (confirmed) {
                    console.log("Removing account...")
                    account.removed.connect(root.finished)
                    account.remove(Account.RemoveCredentials)
                }
            }
        }
    }
}
