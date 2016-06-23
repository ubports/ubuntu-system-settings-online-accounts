/*
 * Copyright (C) 2013-2015 Canonical Ltd.
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
import Ubuntu.Components 1.3
import Ubuntu.OnlineAccounts 0.1
import Ubuntu.OnlineAccounts.Internal 1.0

MainView {
    id: root

    property var request: null
    property var applicationInfo: request.application
    property var providerInfo: request.provider
    property int __createdAccountId: 0

    width: units.gu(48)
    height: units.gu(60)
    automaticOrientation: true

    Component.onCompleted: {
        i18n.domain = "ubuntu-system-settings-online-accounts"
    }

    onRequestChanged: {
        if (accessModel.count === 0 && !accessModel.canCreateAccounts) {
            /* No accounts to authorize */
            request.deny()
            return
        }
        if (!applicationInfo.id && accessModel.count == 1 &&
            applicationInfo.profile == "unconfined") {
            /* Degenerate case: unconfined app making requests with no valid
             * app ID */
            request.allow(accountsModel.get(0, "accountId"))
            return
        }
        loader.active = true
        pageStack.push(mainPage)
    }

    on__CreatedAccountIdChanged: grantAccessIfReady()

    onAllowed: loader.sourceComponent = spinnerComponent

    PageStack {
        id: pageStack

        Page {
            id: mainPage
            title: providerInfo.displayName

            Loader {
                id: loader
                anchors.fill: parent
                active: false
                sourceComponent: ((accessModel.count == 0 && accessModel.canCreateAccounts) ||
                                  applicationInfo.id === "system-settings") ?
                    accountCreationPage : authorizationPage
                onLoaded: {
                    // use this trick to break the sourceComponent binding
                    var tmp = sourceComponent
                    sourceComponent = tmp
                }
            }
        }
    }

    AccountServiceModel {
        id: accountsModel
        service: "global"
        provider: providerInfo.id
        includeDisabled: true
        onCountChanged: root.grantAccessIfReady()

        function indexOfAccount(accountId) {
            for (var i = 0; i < accountsModel.count; i++) {
                if (accountsModel.get(i, "accountId") == accountId)
                    return i
            }
            return -1
        }
    }

    AccessModel {
        id: accessModel
        accountModel: accountsModel
        applicationId: applicationInfo.id

        property bool canCreateAccounts: !providerInfo.isSingleAccount || accountsModel.count === 0
    }

    Component {
        id: accountCreationPage
        AccountCreationPage {
            providerId: providerInfo.id
            onFinished: {
                if (accountId == 0) request.deny()
                /* if an account was created, just remember its ID. when the
                 * accountsModel will notice it we'll proceed with the access
                 * grant */
                else root.__createdAccountId = accountId
            }
        }
    }

    Component {
        id: authorizationPage
        AuthorizationPage {
            model: accessModel
            application: applicationInfo
            provider: providerInfo
            canAddAnotherAccount: accessModel.canCreateAccounts
            onDenied: request.deny()
            onAllowed: root.grantAccess(accountId)
            onCreateAccount: pageStack.push(createAccountPageComponent)
        }
    }

    Component {
        id: createAccountPageComponent
        Page {
            title: providerInfo.displayName
            Loader {
                anchors.fill: parent
                sourceComponent: accountCreationPage
            }
        }
    }

    Component {
        id: spinnerComponent
        Item {
            ActivityIndicator {
                anchors.centerIn: parent
                running: true
            }
        }
    }

    Component {
        id: accountServiceComponent
        AccountService {
            autoSync: false
        }
    }

    Account {
        id: account
        onSynced: accountEnablingDone()
    }

    AccountService {
        id: globalAccountService
        objectHandle: account.accountServiceHandle
        credentials: accountCredentials
        autoSync: false
    }

    AccountServiceModel {
        id: accountServiceModel
        includeDisabled: true
        account: account.objectHandle
    }

    Credentials {
        id: accountCredentials
        credentialsId: globalAccountService.objectHandle != null ? globalAccountService.authData.credentialsId : 0
        onSynced: {
            console.log("Credentials ready (store secret = " + storeSecret + ")")
            if (acl.indexOf(applicationInfo.profile) >= 0) {
                console.log("Application is in ACL: " + acl)
                root.aclDone()
                return
            }
            acl.push(applicationInfo.profile)
            sync()
        }
    }

    function grantAccessIfReady() {
        if (root.__createdAccountId != 0) {
            // If the request comes from system settings, stop here
            if (applicationInfo.id === "system-settings") {
                request.allow(root.__createdAccountId)
                return
            }

            if (accountsModel.indexOfAccount(root.__createdAccountId) >= 0) {
                root.grantAccess(root.__createdAccountId)
                root.__createdAccountId = 0
            }
        }
    }

    function grantAccess(accountId) {
        console.log("granting access to account " + accountId)
        // find the index in the model for this account
        var i = accountsModel.indexOfAccount(accountId)
        if (i < 0) {
            // very unlikely; maybe the account has been deleted in the meantime
            console.log("Account not found:" + accountId)
            request.deny()
            return
        }

        // setting this will trigger the update of the ACL
        account.objectHandle = accountsModel.get(i, "accountHandle")
    }

    function aclDone() {
        console.log("acl done")
        /* now we can enable the application services in the account. */
        for (var i = 0; i < accountServiceModel.count; i++) {
            var accountService = accountServiceComponent.createObject(null, {
                "objectHandle": accountServiceModel.get(i, "accountServiceHandle")
            })
            console.log("Account service account id: " + accountService.accountId)
            var serviceId = accountService.service.id
            if (applicationInfo.services.indexOf(serviceId) >= 0 &&
                !accountService.serviceEnabled) {
                console.log("Enabling service " + serviceId)
                accountService.updateServiceEnabled(true)
            }
            /* The accountService is just a convenience object: all the changes
             * are stored in the account object. So we can destroy this one. */
            accountService.destroy()
        }

        // Store the changes
        account.sync()
    }

    function accountEnablingDone() {
        console.log("account enabling done")
        request.allow(account.accountId)
    }
}
