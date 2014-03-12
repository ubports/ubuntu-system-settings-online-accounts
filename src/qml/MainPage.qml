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

MainView {
    id: root
    width: units.gu(48)
    height: units.gu(60)

    Component.onCompleted: {
        i18n.domain = "ubuntu-system-settings-online-accounts"
        pageStack.push(mainPage)
        /* hack to force the visibility of the back button and make it close
         * the window */
        mainPage.tools.back.visible = true
        mainPage.tools.back.triggered.connect(mainWindow.close)
    }

    PageStack {
        id: pageStack

        Page {
            id: mainPage
            title: i18n.tr("Accounts")

            Loader {
                id: loader
                anchors.fill: parent
                sourceComponent: pluginOptions.provider ? accountCreationPage : normalStartupPage
            }
        }
    }

    Component {
        id: normalStartupPage
        NormalStartupPage {}
    }

    Component {
        id: accountCreationPage
        AccountCreationPage {
            providerId: pluginOptions.provider
        }
    }
}
