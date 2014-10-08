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
import Ubuntu.Components.ListItems 0.1 as ListItem

Flickable {
    id: root

    property variant model
    property variant application
    property variant provider

    signal allowed(int accountId)
    signal denied
    signal createAccount

    Column {
        anchors.left: parent.left
        anchors.right: parent.right

        ListItem.Standard {
            text: i18n.tr("%1 wants to access your %2 account").
                arg(application.displayName).arg(provider.displayName);
        }

        ListItem.ItemSelector {
            id: accountSelector
            anchors.left: parent.left
            anchors.right: parent.right
            width: parent.width - units.gu(4)
            text: "Account"
            model: root.model
            delegate: OptionSelectorDelegate {
                property string modelData: model.displayName
            }
            onDelegateClicked: {
                /* The last item in the model is the "Add another..." label */
                if (model.lastItemText && index == model.count - 1) root.createAccount();
            }
            showDivider: false
        }

        ListItem.SingleControl {
            control: Button {
                width: parent.width - units.gu(4)
                text: i18n.tr("Allow")
                onClicked: root.allowed(root.model.get(accountSelector.selectedIndex, "accountId"))
            }
            showDivider: false
        }

        ListItem.SingleControl {
            control: Button {
                width: parent.width - units.gu(4)
                text: i18n.tr("Don't allow")
                onClicked: root.denied()
            }
            showDivider: false
        }
    }
}
