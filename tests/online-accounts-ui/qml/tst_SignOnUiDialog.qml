import QtQuick 2.9
import QtTest 1.0
import Source 1.0
import Ubuntu.Test 0.1

Item {
    id: root
    width: 400
    height: 400

    QtObject {
        id: request
        property string title
        property string message
        property string loginText

        property bool queryUserName: false
        property string userNameText
        property string userName

        property bool queryPassword: false
        property string passwordText
        property string password

        property string forgotPasswordText
        property url forgotPasswordUrl

        property string registerText
        property url registerUrl

        signal accept()
        signal cancel()
    }

    SignalSpy {
        id: spyAccept
        target: request
        signalName: "accept"
    }

    SignalSpy {
        id: spyCancel
        target: request
        signalName: "cancel"
    }

    Component {
        id: pageComponent
        SignOnUiDialog {}
    }

    UbuntuTestCase {
        name: "SignOnUiDialog"
        when: windowShown

        function tapButton(button) {
            mouseClick(button, button.width / 2, button.height / 2, Qt.LeftButton, 0, 0)
        }

        function createPage() {
            var page = pageComponent.createObject(root)
            spyCancel.clear()
            spyAccept.clear()
            return page
        }

        function test_request_data() {
            return [
                {
                    tag: "only password",
                    request: {
                        queryPassword: true,
                        queryUserName: false,
                        passwordText: "What's your password?",
                        password: "prefilled pw"
                    }
                },
                {
                    tag: "only username",
                    request: {
                        queryPassword: false,
                        queryUserName: true,
                        userNameText: "What's your name?",
                        userName: "Tommy"
                    }
                },
                {
                    tag: "username and password",
                    request: {
                        queryPassword: true,
                        passwordText: "What's your password?",
                        password: "prefilled pw",
                        queryUserName: true,
                        userNameText: "What's your name?",
                        userName: "Tommy"
                    }
                },
                {
                    tag: "forgot password",
                    request: {
                        queryPassword: true,
                        passwordText: "What's your password?",
                        password: "prefilled pw",
                        forgotPasswordUrl: "http://localhost/remember",
                        forgotPasswordText: "Reset your password"
                    }
                }
            ]
        }

        function test_request(data) {
            for (var p in data.request) {
                request[p] = data.request[p]
            }

            var page = createPage()
            verify(page != null)

            var passwordLabel = findChild(page, "passwordLabel")
            verify(passwordLabel != null)
            compare(passwordLabel.visible, request.queryPassword)

            var passwordField = findChild(page, "passwordField")
            verify(passwordField != null)
            compare(passwordField.visible, request.queryPassword)

            if (request.queryPassword) {
                compare(passwordLabel.text, request.passwordText)
                compare(passwordField.text, request.password)
            }

            var userNameLabel = findChild(page, "userNameLabel")
            verify(userNameLabel != null)
            compare(userNameLabel.visible, request.queryUserName)

            var userNameField = findChild(page, "userNameField")
            verify(userNameField != null)
            compare(userNameField.visible, request.queryUserName)

            if (request.queryUserName) {
                compare(userNameLabel.text, request.userNameText)
                compare(userNameField.text, request.userName)
            }

            var forgotPasswordLabel = findChild(page, "forgotPasswordLabel")
            verify(forgotPasswordLabel != null)
            compare(forgotPasswordLabel.visible,
                    request.forgotPasswordUrl.toString() != "")

            page.destroy()
        }

        function test_buttons_data() {
            return [
                {
                    tag: "cancel",
                    buttonName: "cancelButton",
                    expectedCancelCount: 1,
                    expectedAcceptCount: 0
                },
                {
                    tag: "accept",
                    buttonName: "acceptButton",
                    expectedCancelCount: 0,
                    expectedAcceptCount: 1
                }
            ]
        }

        function test_buttons(data) {
            request.queryPassword = true
            request.passwordText = "Enter your password"

            var page = createPage()
            verify(page != null)

            var button = findChild(page, data.buttonName)
            verify(button != null)
            verify(button.visible)

            tapButton(button)
            compare(spyAccept.count, data.expectedAcceptCount)
            compare(spyCancel.count, data.expectedCancelCount)

            page.destroy()
        }
    }
}
