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
import Ubuntu.Components.ListItems 0.1 as ListItem

Flickable {
    id: root

    property variant model
    property variant application
    property variant provider
    property bool canAddAnotherAccount: true

    signal allowed(int accountId)
    signal denied
    signal createAccount

    Column {
        id: topColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: units.gu(1)
        spacing: units.gu(1)

        Label {
            objectName: "msgLabel"
            anchors.left: parent.left
            anchors.right: parent.right
            text: i18n.tr("%1 wants to access your %2 account").
                arg(application.displayName).arg(provider.displayName);
            wrapMode: Text.WordWrap
        }

        Label {
            id: accountLabel
            objectName: "accountLabel"
            anchors.left: parent.left
            anchors.right: parent.right
            visible: model.count == 1
            horizontalAlignment: Text.AlignHCenter
            text: model.get(0, "displayName")
        }

        OptionSelector {
            id: accountSelector
            objectName: "accountSelector"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: units.gu(1)
            anchors.rightMargin: units.gu(1)
            visible: !accountLabel.visible
            model: root.model
            delegate: OptionSelectorDelegate {
                property string modelData: model.displayName
            }
        }
    }

    Column {
        anchors.top: topColumn.bottom
        anchors.margins: units.gu(2)
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: units.gu(1)

        Button {
            objectName: "allowButton"
            anchors.left: parent.left
            anchors.right: parent.right
            text: i18n.tr("Allow")
            onClicked: root.allowed(root.model.get(accountSelector.selectedIndex, "accountId"))
        }

        Button {
            objectName: "addAnotherButton"
            anchors.left: parent.left
            anchors.right: parent.right
            visible: canAddAnotherAccount
            text: i18n.tr("Add another account…")
            onClicked: root.createAccount()
        }

        Button {
            objectName: "denyButton"
            anchors.left: parent.left
            anchors.right: parent.right
            text: i18n.tr("Don't allow")
            onClicked: root.denied()
        }
    }
}
