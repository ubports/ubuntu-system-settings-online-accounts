import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.ListItems 0.1 as ListItem
import Ubuntu.OnlineAccounts.Plugin 1.0

MainView {
    id: root

    property var signonRequest: request

    width: units.gu(60)
    height: units.gu(90)

    Page {
        title: signonRequest.title

        Column {
            id: loginForm
            anchors.left: parent.left
            anchors.right: parent.right

            ListItem.Caption {
                visible: signonRequest.message !== ""
                text: signonRequest.message
            }

            Label {
                id: userNameLabel
                visible: signonRequest.queryUserName
                text: signonRequest.userNameText
            }

            TextField {
                id: userNameField
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
                    text: signonRequest.passwordText
                }

                TextField {
                    id: passwordField
                    text: signonRequest.password
                    echoMode: TextInput.Password
                    onTextChanged: signonRequest.password = text
                    Keys.onReturnPressed: signonRequest.accept()
                } 
                Label {
                    visible: signonRequest.forgotPasswordUrl.toString() !== ""
                    text: "<a href=\"" + signonRequest.forgotPasswordUrl + "\">" +
                        signonRequest.forgotPasswordText + "</a>"
                }
            }
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: loginForm.bottom
            anchors.bottom: pageFooter.top

            Row {
                height: childrenRect.height
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                spacing: units.gu(1)

                Button {
                    text: i18n.dtr("ubuntu-system-settings-online-accounts", "Cancel")
                    width: (parent.width / 2) - 0.5 * parent.spacing
                    onClicked: signonRequest.cancel()
                }

                Button {
                    text: signonRequest.loginText
                    width: (parent.width / 2) - 0.5 * parent.spacing
                    onClicked: signonRequest.accept()
                }
            }
        }

        Item {
            id: pageFooter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: osk.top
            visible: signonRequest.registerUrl.toString() !== ""

            ListItem.ThinDivider {
                anchors.bottom: registerUrlLabel.top
            }

            Label {
                anchors.bottom: parent.bottom
                id: registerUrlLabel
                text: "<a href=\"" + signonRequest.registerUrl + "\">" +
                    signonRequest.registerText + "</a>"
            }
        }

        KeyboardRectangle {
            id: osk
        }
    }
}
