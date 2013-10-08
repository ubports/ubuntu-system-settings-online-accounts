import QtQuick 2.0
import Ubuntu.Components 0.1

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

    Button {
        id: cancelButton
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        text: i18n.dtr("ubuntu-system-settings-online-accounts", "Cancel")
        width: parent.width - units.gu(4)
        onClicked: request.cancel()
    }
}
