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
import Ubuntu.Components.ListItems 1.3 as ListItem
import Ubuntu.OnlineAccounts 0.1

ListItem.Subtitled {
    property variant accountHandle
    property variant globalServiceHandle
    property variant __editPage: null
    property bool running: false

    iconSource: globalService.provider.iconName.indexOf("/") === 0 ?
        globalService.provider.iconName : "image://theme/" + globalService.provider.iconName
    progression: true
    opacity: globalService.enabled ? 1 : 0.5

    resources: [
        AccountService {
            id: globalService
            objectHandle: globalServiceHandle
        },
        Component {
            id: accountEditPage
            AccountEditPage {}
        }
    ]

    onClicked: {
        __editPage = accountEditPage.createObject(null, {
            "accountHandle": accountHandle })
        __editPage.finished.connect(__onEditFinished)
        pageStack.addPageToNextColumn(mainAccountsPage, __editPage)
        running = true;
    }

    function __onEditFinished() {
        __editPage.destroy(1000)
        pageStack.removePages(__editPage)
        __editPage = null
        running = false
    }
}
