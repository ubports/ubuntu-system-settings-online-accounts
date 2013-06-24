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
import Ubuntu.Components.ListItems 0.1 as ListItem

Dialog {
    id: root

    property string accountName
    property bool confirmed: false

    signal closed

    title: i18n.dtr("ubuntu-system-settings-online-accounts", "Remove account")
    text: i18n.dtr("ubuntu-system-settings-online-accounts", "The %1 account will be removed only from your phone. You can add it again later.").arg(accountName)

    Button {
        text: i18n.dtr("ubuntu-system-settings-online-accounts", "Remove")
        onClicked: setConfirmed(true)
    }

    Button {
        text: i18n.dtr("ubuntu-system-settings-online-accounts", "Cancel")
        onClicked: setConfirmed(false)
    }

    function setConfirmed(isConfirmed) {
        confirmed = isConfirmed
        PopupUtils.close(root)
        closed()
    }
}
