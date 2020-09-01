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

Flickable {
    id: root

    property var model
    property var application
    property var provider
    property bool canAddAnotherAccount: true

    signal allowed(int accountId)
    signal denied
    signal createAccount

    anchors.fill: parent
    contentHeight: bottomColumn.y + bottomColumn.height + bottomColumn.anchors.margins

    Column {
        id: topColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: units.gu(1)
        spacing: units.gu(1)

        ProportionalShape {
            id: iconShape
            anchors.horizontalCenter: parent.horizontalCenter
            aspect: UbuntuShape.DropShadow
            width: units.gu(8)
            source: Image {
                sourceSize.width: iconShape.width
                sourceSize.height: iconShape.height
                source: application.icon.indexOf("/") === 0 ?
                    "file://" + application.icon : "image://theme/" + application.icon
            }
        }

        Column {
            anchors { left: parent.left; right: parent.right; margins: units.gu(1) }

            Label {
                objectName: "appLabel"
                anchors.left: parent.left
                anchors.right: parent.right
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: application.displayName
                wrapMode: Text.Wrap
                maximumLineCount: 2
            }
            Label {
                objectName: "pkgLabel"
                anchors.left: parent.left
                anchors.right: parent.right
                horizontalAlignment: Text.AlignHCenter
                color: theme.palette.normal.backgroundText
                elide: Text.ElideMiddle
                text: application.displayId
            }
        }

        Label {
            objectName: "msgLabel"
            anchors.left: parent.left
            anchors.right: parent.right
            text: i18n.tr("wants to access your %2 account").
                arg(provider.displayName);
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
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
        id: bottomColumn
        anchors.top: topColumn.bottom
        anchors.margins: units.gu(2)
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: units.gu(1)

        Button {
            objectName: "allowButton"
            anchors.left: parent.left
            anchors.right: parent.right
            color: theme.palette.normal.positive
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
