import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem

Page {
    id: root

    Loader {
        id: loader
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: cancelButton.top
            bottomMargin: Math.max(osk.height - cancelButton.height, 0)
        }
        focus: true
        sourceComponent: browserComponent
    }

    ListItem.SingleControl {
        id: cancelButton
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        control: Button {
            text: i18n.dtr("ubuntu-system-settings-online-accounts", "Cancel")
            width: parent.width - units.gu(4)
            onClicked: request.cancel()
        }
        showDivider: false
    }
}
