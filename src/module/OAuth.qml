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
import Ubuntu.OnlineAccounts 0.1

Column {
    id: root

    property variant authReply
    property bool isNewAccount: false

    signal authenticated(variant reply)
    signal authenticationError(variant error)
    signal finished

    anchors.left: parent.left
    anchors.right: parent.right

    Component.onCompleted: {
        isNewAccount = (account.accountId === 0)
        authenticate()
    }

    Credentials {
        id: creds
        caption: account.provider.id
        acl: ["*"]
        onCredentialsIdChanged: root.credentialsStored()
    }

    AccountService {
        id: globalAccountSettings
        objectHandle: account.accountServiceHandle
        credentials: creds
        autoSync: false

        onAuthenticated: {
            authReply = reply
            root.authenticated(reply)
        }
        onAuthenticationError: root.authenticationError(error)
    }

    Button {
        anchors.left: parent.left
        anchors.right: parent.right
        text: i18n.dtr("uoa-setup", "Cancel")
        onClicked: root.cancel()
    }

    function authenticate() {
        console.log("Authenticating...")
        creds.sync()
    }

    function credentialsStored() {
        console.log("Credentials stored, id: " + creds.credentialsId)
        if (creds.credentialsId == 0) return
        globalAccountSettings.authenticate(null)
    }

    function cancel() {
        if (isNewAccount && creds.credentialsId != 0) {
            console.log("Removing credentials...")
            creds.remove()
            creds.removed.connect(finished)
        } else {
            finished()
        }
    }

    function getUserName(reply) {
        /* This should work for OAuth 1.0a; for OAuth 2.0 this function needs
         * to be reimplemented */
        if ('ScreenName' in reply) return reply.ScreenName
        else if ('UserId' in reply) return reply.UserId
        return ''
    }

    onAuthenticated: {
        var userName = getUserName(reply)

        console.log("UserName: " + userName)
        if (userName != '') account.updateDisplayName(userName)
        account.synced.connect(finished)
        account.sync()
    }

    onAuthenticationError: root.cancel()
}
