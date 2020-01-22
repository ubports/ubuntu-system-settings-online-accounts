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
import Ubuntu.OnlineAccounts 0.1
import Ubuntu.OnlineAccounts.Plugin 1.0

Page {
    id: root

    property variant accountHandle

    signal finished

    header: PageHeader {
        title: account.provider.displayName
        flickable: flick
    }

    Account {
        id: account
        objectHandle: accountHandle
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: options.height

        Options {
            id: options
            onFinished: {
                root.finished()
            }
        }
    }
}
