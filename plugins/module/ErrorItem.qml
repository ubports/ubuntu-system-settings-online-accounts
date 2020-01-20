/*
 * Copyright (C) 2015 Canonical Ltd.
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
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem

Item {
    id: root

    signal retryRequested()

    Column {
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left; right: parent.right
        }
        spacing: units.gu(2)

        Label {
            anchors { left: parent.left; right: parent.right }
            text: i18n.dtr("ubuntu-system-settings-online-accounts", "This service is not available right now. Try again later.")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: i18n.dtr("ubuntu-system-settings-online-accounts", "Try Again")
            onClicked: root.retryRequested()
        }
    }
}
