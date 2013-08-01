import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.OnlineAccounts.Client 0.1

Rectangle {
    width: 400
    height: 300

    Button {
        anchors.centerIn: parent
        text: "Create Facebook account"
        onClicked: setup.exec()
    }

    Setup {
        id: setup
        providerId: "facebook"
    }
}
