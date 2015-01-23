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
import Ubuntu.OnlineAccounts 0.1
import Ubuntu.OnlineAccounts.Plugin 1.0

Item {
    id: root

    /* To override the parameters coming from the .provider file: */
    property variant authenticationParameters: {}
    /* To override the default access control list: */
    property variant accessControlList: ["unconfined"]

    property variant authReply
    property bool isNewAccount: false
    property variant __account: account
    property bool __isAuthenticating: false
    property alias globalAccountService: globalAccountSettings
    property bool loading: loader.status == Loader.Null || loader.status == Loader.Loading

    signal authenticated(variant reply)
    signal authenticationError(variant error)
    signal finished

    anchors.fill: parent

    Component.onCompleted: {
        isNewAccount = (account.accountId === 0)
        enableAccount()
        authenticate()
    }

    RequestHandler {
        id: requestHandler
        onRequestChanged: {
            if (request) {
                console.log("RequestHandler captured request!")
                loader.setSource("WebView.qml", {
                    "signonRequest": request
                })
            } else {
                console.log("Request destroyed!")
                loader.source = ""
            }
        }
    }

    Credentials {
        id: creds
        caption: account.provider.id
        acl: accessControlList
        onCredentialsIdChanged: root.credentialsStored()
    }

    AccountService {
        id: globalAccountSettings
        objectHandle: account.accountServiceHandle
        credentials: creds
        autoSync: false

        onAuthenticated: {
            __isAuthenticating = false
            authReply = reply
            root.authenticated(reply)
        }
        onAuthenticationError: {
            __isAuthenticating = false
            root.authenticationError(error)
        }
    }

    AccountServiceModel {
        id: accountServices
        includeDisabled: true
        account: __account.objectHandle
    }

    ListItem.Base {
        visible: loading
        height: units.gu(7)
        showDivider: false
        anchors.top: parent.top

        Item {
            height: units.gu(5)
            width: units.gu(30)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.margins: units.gu(1)

            ActivityIndicator {
                id: loadingIndicator
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: units.gu(5)
                running: loading
                z: 1
            }
            Label {
                text: i18n.dtr("ubuntu-system-settings-online-accounts", "Loading…")
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: loadingIndicator.right
                anchors.leftMargin: units.gu(3)
            }
        }
    }

    Loader {
        id: loader
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: Qt.inputMethod.visible ? osk.top : cancelButton.top
        }
        focus: true
        visible: !loading
    }

    KeyboardRectangle {
        id: osk
    }

    ListItem.SingleControl {
        id: cancelButton
        anchors.bottom: parent.bottom
        showDivider: false
        control: Button {
            text: i18n.dtr("ubuntu-system-settings-online-accounts", "Cancel")
            width: parent.width - units.gu(4)
            onClicked: root.cancel()
        }
    }

    Component {
        id: accountServiceComponent
        AccountService {
            autoSync: false
        }
    }

    function authenticate() {
        console.log("Authenticating...")
        creds.sync()
    }

    function credentialsStored() {
        console.log("Credentials stored, id: " + creds.credentialsId)
        if (creds.credentialsId == 0) return
        var parameters = {}
        parameters[requestHandler.matchKey] = requestHandler.matchId
        parameters["providerId"] = account.provider.id
        for (var p in authenticationParameters) {
            parameters[p] = authenticationParameters[p]
        }
        __isAuthenticating = true
        globalAccountSettings.authenticate(parameters)
    }

    function cancel() {
        if (__isAuthenticating) {
            /* This will cause the authentication to fail, and this method will
             * be invoked again to delete the credentials. */
            globalAccountSettings.cancelAuthentication()
            return
        }
        if (isNewAccount && creds.credentialsId != 0) {
            console.log("Removing credentials...")
            creds.remove()
            creds.removed.connect(finished)
        } else {
            finished()
        }
    }

    function enableAccount() {
        globalAccountSettings.updateServiceEnabled(true)
    }

    function getUserName(reply) {
        /* This should work for OAuth 1.0a; for OAuth 2.0 this function needs
         * to be reimplemented */
        if ('ScreenName' in reply) return reply.ScreenName
        else if ('UserId' in reply) return reply.UserId
        return ''
    }

    /* reimplement this function in plugins in order to perform some actions
     * before quitting the plugin */
    function completeCreation(reply) {
        var userName = getUserName(reply)

        console.log("UserName: " + userName)
        if (userName != '') account.updateDisplayName(userName)
        account.synced.connect(finished)
        account.sync()
    }

    onAuthenticated: completeCreation(reply)

    onAuthenticationError: root.cancel()

    onFinished: loading = false
}
