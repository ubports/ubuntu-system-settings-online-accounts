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
import Ubuntu.OnlineAccounts 0.1

Item {
    id: root

    property string providerId
    property var flickable: null

    signal finished(int accountId)

    Account {
        id: account
        objectHandle: Manager.createAccount(providerId)
    }

    Loader {
        id: loader
        property var account: account

        anchors.fill: parent
        source: localQmlPluginPath + providerId + "/Main.qml"
        onLoaded: checkFlickable()

        onStatusChanged: {
            if (loader.status == Loader.Error) {
                loader.source = systemQmlPluginPath + providerId + "/Main.qml"
            }
        }

        Connections {
            target: loader.item
            onFinished: {
                console.log("====== PLUGIN FINISHED ======")
                finished(account.accountId)
            }
        }

        function checkFlickable() {
            if (item.hasOwnProperty("flickable")) {
                root.flickable = item.flickable
            }
        }
    }
}
