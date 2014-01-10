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
            /* FIXME: Work around a weird issue with Qt.resolvedUrl where the
               "tests" part of the path is repeated, which fails the comparison.
               Only check the end of the source path to make sure it's changed.*/
            verify(page.exportedLoader.source.toString().substr(-43) ==
                 Qt.resolvedUrl(systemQmlPluginPath +
                    "testPlugin/Main.qml").toString().substr(-43))
            page.destroy()
        }
    }
}
