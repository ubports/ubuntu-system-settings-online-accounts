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

Flickable {
    id: root

    property variant accountsModel

    contentHeight: contentItem.childrenRect.height

    Column {
        anchors.left: parent.left
        anchors.right: parent.right

        Repeater {
            model: accountsModel

            delegate: AccountItem {
                text: providerName
                subText: displayName
                globalServiceHandle: accountService
            }
        }

        Button {
            anchors.left: parent.left
            anchors.right: parent.right
            text: i18n.dtr(plugin.translations, "Add account…")
            onClicked: pageStack.push(newAccountPage)
        }

        AddAccountLabel {}
    }

    Component {
        id: newAccountPage
        NewAccountPage {}
    }
}
