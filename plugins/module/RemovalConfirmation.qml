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
import Ubuntu.Components.Popups 1.3

Dialog {
    id: root

    property string accountName
    property bool confirmed: false

    signal closed

    title: i18n.dtr("ubuntu-system-settings-online-accounts", "Remove account")
    text: i18n.dtr("ubuntu-system-settings-online-accounts", "The %1 account will be removed only from your phone. You can add it again later.").arg(accountName)

    onVisibleChanged: if (!visible) closed()

    Button {
        text: i18n.dtr("ubuntu-system-settings-online-accounts", "Remove")
        color: theme.palette.normal.negative
        onClicked: setConfirmed(true)
    }

    Button {
        text: i18n.dtr("ubuntu-system-settings-online-accounts", "Cancel")
        onClicked: setConfirmed(false)
    }

    function setConfirmed(isConfirmed) {
        confirmed = isConfirmed
        PopupUtils.close(root)
    }
}
