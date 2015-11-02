import QtQuick 2.0
import Ubuntu.Components 1.3
import Ubuntu.Components.Popups 1.3
import Ubuntu.Components.ListItems 1.3 as ListItem

Item {
    id: root

    property var signonRequest: request

    width: units.gu(60)
    height: units.gu(90)

    Component.onCompleted: dialog.show()

    Dialog {
        id: dialog
        objectName: "signOnUiDialog"
        title: signonRequest.title
        text: signonRequest.message

        Label {
            id: userNameLabel
            objectName: "userNameLabel"
            visible: signonRequest.queryUserName
            text: signonRequest.userNameText
        }

        TextField {
            id: userNameField
            objectName: "userNameField"
            visible: signonRequest.queryUserName
            text: signonRequest.userName
            onTextChanged: signonRequest.userName = text
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            visible: signonRequest.queryPassword

            Label {
                id: passwordLabel
                objectName: "passwordLabel"
                text: signonRequest.passwordText
            }

            TextField {
                id: passwordField
                objectName: "passwordField"
                text: signonRequest.password
                echoMode: TextInput.Password
                onTextChanged: signonRequest.password = text
                Keys.onReturnPressed: signonRequest.accept()
            } 
            Label {
                objectName: "forgotPasswordLabel"
                visible: signonRequest.forgotPasswordUrl.toString() !== ""
                text: "<a href=\"" + signonRequest.forgotPasswordUrl + "\">" +
                    signonRequest.forgotPasswordText + "</a>"
            }
        }

        Item {
            id: pageFooter
            anchors.left: parent.left
            anchors.right: parent.right
            visible: signonRequest.registerUrl.toString() !== ""

            ListItem.ThinDivider {
                anchors.bottom: registerUrlLabel.top
            }

            Label {
                id: registerUrlLabel
                objectName: "registerUrlLabel"
                anchors.bottom: parent.bottom
                text: "<a href=\"" + signonRequest.registerUrl + "\">" +
                    signonRequest.registerText + "</a>"
            }
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right

            Button {
                objectName: "cancelButton"
                anchors.left: parent.left
                text: i18n.dtr("ubuntu-system-settings-online-accounts", "Cancel")
                width: (parent.width / 2) - 0.5 * units.gu(1)
                onClicked: signonRequest.cancel()
            }

            Button {
                objectName: "acceptButton"
                anchors.right: parent.right
                text: signonRequest.loginText
                width: (parent.width / 2) - 0.5 * units.gu(1)
                onClicked: signonRequest.accept()
            }
        }
    }
}
