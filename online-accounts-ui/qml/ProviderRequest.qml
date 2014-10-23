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
import Ubuntu.OnlineAccounts.Internal 1.0

MainView {
    id: root

    property variant applicationInfo: application
    property variant providerInfo: provider
    property bool wasDenied: false
    property int __createdAccountId: 0

    signal denied
    signal allowed(int accountId)

    width: units.gu(48)
    height: units.gu(60)
    useDeprecatedToolbar: false
    automaticOrientation: true

    Component.onCompleted: {
        i18n.domain = "ubuntu-system-settings-online-accounts"
        if (accessModel.count === 0) {
            /* No accounts to authorize */
            denied()
            return
        }
        loader.active = true
        pageStack.push(mainPage)
    }

    on__CreatedAccountIdChanged: grantAccessIfReady()
    onDenied: wasDenied = true

    PageStack {
        id: pageStack

        Page {
            id: mainPage
            title: providerInfo.displayName

            Loader {
                id: loader
                anchors.fill: parent
                active: false
                sourceComponent: ((accessModel.count == 0 && accessModel.canCreateAccounts()) ||
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

        function canCreateAccounts() {
            if (!providerInfo.isSingleAccount) return true
            return accountsModel.count === 0
        }
    }

    Component {
        id: accountCreationPage
        AccountCreationPage {
            providerId: providerInfo.id
            onFinished: {
                if (accountId == 0) root.denied()
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
            onDenied: root.denied()
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
                root.allowed(root.__createdAccountId)
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
            root.denied()
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
        allowed(account.accountId)
    }
}
