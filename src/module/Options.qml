/*
 * Copyright (C) 2013 Canonical Ltd.
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

import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.Popups 0.1
import Ubuntu.OnlineAccounts 0.1

Column {
    id: root

    property variant __account: account

    signal finished

    anchors.left: parent.left
    anchors.right: parent.right

    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        height: idLabel.height
        Label {
            id: idLabel
            text: i18n.dtr("uoa-setup", "ID:")
        }
        Label {
            anchors.left: idLabel.right
            anchors.right: parent.right
            horizontalAlignment: Text.AlignRight
            text: account.displayName
        }
    }

    ServiceSwitches {
        account: __account
        enabled: __account.enabled
        opacity: enabled ? 1 : 0.5
    }

    Button {
        id: removeBtn
        anchors.left: parent.left
        anchors.right: parent.right
        text: i18n.dtr("uoa-setup", "Remove Account…")
        onClicked: PopupUtils.open(removalConfirmationComponent, removeBtn)
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
