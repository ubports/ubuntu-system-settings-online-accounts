/*
 * Copyright (C) 2016 Canonical Ltd.
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

ListItem.Base {
    property alias checked: control.checked
    property alias text: label.text
    property alias subText: subLabel.text

    Item {
        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
        height: childrenRect.height + label.anchors.topMargin + subLabel.anchors.bottomMargin

        Label {
            id: label
            anchors { left: parent.left; right: control.left; top: parent.top }
            elide: Text.ElideRight
            color: theme.palette.selected.backgroundText
        }

        Label {
            id: subLabel
            anchors { left: parent.left; right: control.left; top: label.bottom }
            color: theme.palette.normal.backgroundText
            elide: Text.ElideRight
            textSize: Label.Small
        }

        Switch {
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
            id: control
        }
    }
}
