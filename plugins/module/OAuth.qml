/*
 * Copyright (C) 2013-2016 Canonical Ltd.
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
import Ubuntu.Components.Popups 1.3
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
    property string userAgent

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
                    "signonRequest": request,
                    "userAgent": userAgent
                })
            } else {
                console.log("Request destroyed!")
                /* Destroying the WebView causes WebEngine to crash (2019-12-16)
                 * https://github.com/ubports/morph-browser/issues/271
                 */
                //loader.source = ""
                loader.item.visible = false
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
        visible: loading && !errorItem.visible
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

    ErrorItem {
        id: errorItem
        anchors { fill: parent; margins: units.gu(4) }
        visible: false
        onRetryRequested: {
            root.credentialsStored()
            visible = false
        }
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

    AccountServiceModel {
        id: possiblyDuplicateAccounts
        service: "global"
        provider: __account.provider.id
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

    function getUserName(reply, callback) {
        /* This should work for OAuth 1.0a; for OAuth 2.0 this function needs
         * to be reimplemented */
        if ('ScreenName' in reply) return reply.ScreenName
        else if ('UserId' in reply) return reply.UserId
        return ''
    }

    function accountIsDuplicate(userName) {
        var model = possiblyDuplicateAccounts
        for (var i = 0; i < model.count; i++) {
            if (model.get(i, "displayName") == userName)
                return true
        }
        return false
    }

    function __gotUserName(userName, reply) {
        console.log("UserName: " + userName)
        if (userName != '') {
            if (accountIsDuplicate(userName)) {
                var dialog = PopupUtils.open(Qt.resolvedUrl("DuplicateAccount.qml"))
                dialog.closed.connect(cancel)
                return
            }
            account.updateDisplayName(userName)
        }
        beforeSaving(reply)
    }

    function saveAccount() {
        account.synced.connect(finished)
        account.sync()
    }

    /* reimplement this function in plugins in order to perform some actions
     * before quitting the plugin */
    function beforeSaving(reply) {
        saveAccount()
    }

    function __getUserNameAndSave(reply) {
        /* If the completeCreation function is defined, run it */
        if (typeof(completeCreation) == "function") {
            console.warn("The completeCreation method is deprecated; use getUserName() or beforeSaving() instead")
            completeCreation(reply)
            return
        }

        var userName = getUserName(reply, function(name, error) {
            if (error) {
                console.warn("Error while getting username: " + error)
                cancel()
            } else {
                __gotUserName(name, reply)
            }
        })
        if (typeof(userName) == "string") {
            __gotUserName(userName, reply)
        } else if (userName === false) {
            cancel()
            return
        }
        // otherwise (userName === true), wait for the callback to be invoked
    }

    onAuthenticated: __getUserNameAndSave(reply)

    onAuthenticationError: {
        console.log("Authentication error, code " + error.code)
        if (error.code == AccountService.NetworkError) {
            console.log("Network error")
            errorItem.visible = true
            return
        }
        root.cancel()
    }

    onFinished: loading = false
}
