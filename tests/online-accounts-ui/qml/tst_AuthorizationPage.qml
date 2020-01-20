import QtQuick 2.9
import QtTest 1.0
import Source 1.0
import Ubuntu.OnlineAccounts 0.1
import Ubuntu.Test 0.1

Item {
    id: root
    width: 400
    height: 400

    AccountServiceModel {
        id: accountsModel
        includeDisabled: true
        provider: "cool"
        service: "coolmail"
    }

    SignalSpy {
        id: spyDenied
        signalName: "denied"
    }

    SignalSpy {
        id: spyCreateAccount
        signalName: "createAccount"
    }

    SignalSpy {
        id: spyAllowed
        signalName: "allowed"
    }

    SignalSpy {
        id: spyAccount
        signalName: "synced"
    }

    Component {
        id: accountComponent
        Account {}
    }

    Component {
        id: pageComponent
        AuthorizationPage {
            model: accountsModel
        }
    }

    UbuntuTestCase {
        name: "AuthorizationPage"
        when: windowShown

        function createAccount(displayName) {
            spyAccount.clear()
            var accountHandle = Manager.createAccount("cool")
            var account = accountComponent.createObject(null, {
                "objectHandle": accountHandle
            })
            account.updateDisplayName(displayName)
            account.updateEnabled(false)
            spyAccount.target = account
            account.sync()
            if (spyAccount.count === 0) {
                spyAccount.wait()
            }
            compare(spyAccount.count, 1)
            account.destroy(1000)
        }

        function tapButton(button) {
            mouseClick(button, button.width / 2, button.height / 2, Qt.LeftButton, 0, 0)
        }

        function createPage(params) {
            var page = pageComponent.createObject(root, params)
            spyAllowed.clear()
            spyAllowed.target = page
            spyDenied.clear()
            spyDenied.target = page
            spyCreateAccount.clear()
            spyCreateAccount.target = page
            return page
        }

        function test_1_one_account() {
            createAccount("My account")
            var page = createPage({
                "application": {
                    "displayName": "My app",
                    "displayId": "com.ubuntu/MyApp",
                },
                "provider": { "displayName": "My provider" }
            })
            verify(page != null)

            var label = findChild(page, "appLabel")
            verify(label != null)
            compare(label.text, "My app")

            label = findChild(page, "pkgLabel")
            verify(label != null)
            compare(label.text, "com.ubuntu/MyApp")

            label = findChild(page, "msgLabel")
            verify(label != null)
            compare(label.text, "wants to access your My provider account")

            var accountLabel = findChild(page, "accountLabel")
            verify(accountLabel != null)
            compare(accountLabel.visible, true)
            compare(accountLabel.text, "My account")

            // Check that the selector is *not* visible
            var accountSelector = findChild(page, "accountSelector")
            verify(accountSelector != null)
            compare(accountSelector.visible, false)

            // Test the buttons
            var allowButton = findChild(page, "allowButton")
            verify(allowButton != null)
            compare(allowButton.visible, true)
            tapButton(allowButton)
            compare(spyAllowed.count, 1)
            compare(spyAllowed.signalArguments[0][0], accountsModel.get(0, "accountId"))
            compare(spyDenied.count, 0)
            compare(spyCreateAccount.count, 0)

            var denyButton = findChild(page, "denyButton")
            verify(denyButton != null)
            compare(denyButton.visible, true)
            tapButton(denyButton)
            compare(spyAllowed.count, 1)
            compare(spyDenied.count, 1)
            compare(spyCreateAccount.count,0)

            page.destroy()
        }

        function test_2_add_another_data() {
            return [
                { tag: "with button", canAdd: true },
                { tag: "without button", canAdd: false }
            ]
        }

        function test_2_add_another(data) {
            var page = createPage({
                "application": { "displayName": "My app" },
                "provider": { "displayName": "My provider" },
                "canAddAnotherAccount": data.canAdd
            })
            verify(page != null)

            var addAnotherButton = findChild(page, "addAnotherButton")
            verify(addAnotherButton != null)
            compare(addAnotherButton.visible, data.canAdd)
            if (addAnotherButton.visible) {
                tapButton(addAnotherButton)
                compare(spyAllowed.count, 0)
                compare(spyDenied.count, 0)
                compare(spyCreateAccount.count, 1)
            }

            page.destroy()
        }

        function test_3_many_accounts_data() {
            return [
                { tag: "first account", index: 0 },
                { tag: "second account", index: 1 }
            ]
        }

        function test_3_many_accounts(data) {
            createAccount("Your account")
            var page = createPage({
                "application": { "displayName": "My app" },
                "provider": { "displayName": "My provider" }
            })
            verify(page != null)

            var accountLabel = findChild(page, "accountLabel")
            verify(accountLabel != null)
            compare(accountLabel.visible, false)

            var accountSelector = findChild(page, "accountSelector")
            verify(accountSelector != null)
            compare(accountSelector.visible, true)
            accountSelector.selectedIndex = data.index

            var allowButton = findChild(page, "allowButton")
            verify(allowButton != null)
            compare(allowButton.visible, true)
            tapButton(allowButton)
            compare(spyAllowed.count, 1)
            compare(spyAllowed.signalArguments[0][0],
                    accountsModel.get(data.index, "accountId"))
            compare(spyDenied.count, 0)
            compare(spyCreateAccount.count, 0)

            page.destroy()
        }
    }
}
