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
import Ubuntu.Components 1.1
import Ubuntu.Components.ListItems 1.0 as ListItem
import Ubuntu.OnlineAccounts 0.1

Column {
    id: root

    signal providerClicked(string providerId)

    anchors.left: parent.left
    anchors.right: parent.right

    ProviderModel {
        id: providerModel
    }

    Repeater {
        id: repeater
        model: providerModel

        delegate: ListItem.Standard {
            id: provider
            text: displayName
            enabled: accountModel === null || accountModel.count === 0
            iconSource: model.iconName.indexOf("/") === 0 ?
                model.iconName : "image://theme/" + model.iconName
            progression: false
            onTriggered: {
                activated = true;
                root.providerClicked(providerId);
            }
            property var accountModel: isSingleAccount ? createAccountModel(providerId) : null
            property bool activated: false

            ActivityIndicator {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: units.gu(2)
                running: provider.activated
            }

            function createAccountModel(providerId) {
                return accountModelComponent.createObject(this, {
                    "provider": providerId
                })
            }
        }
    }

    Component {
        id: accountModelComponent
        AccountServiceModel {
            includeDisabled: true
        }
    }

    function clearPressedButtons() {
        for (var i = 0; i < repeater.count; i++) {
            repeater.itemAt(i).activated = false
        }
    }
}
