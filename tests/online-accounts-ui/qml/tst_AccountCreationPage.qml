import QtQuick 2.0
import QtTest 1.0
import Source 1.0

Item {
    property string localQmlPluginPath: "../../tests/online-accounts-ui/qml/"
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

        function test_fallback() {
            localQmlPluginPath = "/dummy/path/"
            var page = pageComponent.createObject(null, {
                "providerId": "testPlugin" })
            // If flickable is set then the plugin was loaded correctly
            verify(page.flickable != null)
            page.destroy()
        }
    }
}
