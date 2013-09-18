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

Page {
    id: root

    property string providerId

    title: account.provider.displayName

    Account {
        id: account
        objectHandle: Manager.createAccount(providerId)
    }

    Loader {
        id: loader
        property var account: account

        anchors.fill: parent
        source: qmlPluginPath + providerId + "/Main.qml"
        onLoaded: checkFlickable()

        Connections {
            target: loader.item
            onFinished: {
                console.log("====== PLUGIN FINISHED ======")
                pageStack.pop()
                pageStack.pop()
            }
        }

        function checkFlickable() {
            if (item.hasOwnProperty("flickable")) {
                root.flickable = item.flickable
            }
        }
    }
}
