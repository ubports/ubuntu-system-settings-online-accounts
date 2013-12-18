import QtQuick 2.0
import QtTest 1.0
import Source 1.0

Item {
    property string systemQmlPluginPath: "../../tests/online-accounts-ui/qml/"
    width: 200
    height: 200

    Component {
        id: pageComponent
        AccountCreationPage {}
    }

    TestCase {
        name: "AccountCreationPage"

        function test_flickable() {
            var page = pageComponent.createObject(null, {
                "providerId": "testPlugin" })
            verify(page.flickable != null)
            page.destroy()
        }
    }
}
