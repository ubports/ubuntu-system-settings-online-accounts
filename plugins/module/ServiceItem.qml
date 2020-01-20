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

Column {
    property variant accountServiceHandle

    signal applicationAdded(string applicationId)
    signal applicationRemoved(string applicationId)

    anchors.left: parent.left
    anchors.right: parent.right

    Repeater {
        resources: AccountService {
            id: accountService
            objectHandle: accountServiceHandle
        }

        model: ApplicationModel {
            service: accountService.service.id
        }

        delegate: ServiceItemBase {
            text: model.displayName ? model.displayName : model.applicationId
            subText: ApplicationManager.applicationInfo(model.applicationId, "unconfined").displayId
            iconSource: model.iconName.indexOf("/") === 0 ?
                model.iconName : "image://theme/" + model.iconName
            checked: accountService.serviceEnabled
            onCheckedChanged: {
                if (checked != accountService.serviceEnabled) {
                    if (checked) {
                        applicationAdded(model.applicationId)
                    } else {
                        applicationRemoved(model.applicationId)
                    }
                    accountService.updateServiceEnabled(checked)
                }
            }
        }
    }
}
