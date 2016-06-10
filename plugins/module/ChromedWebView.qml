import QtQuick 2.0
import Ubuntu.Components 1.3
import Ubuntu.Web 0.2

WebView {
    id: root

    /* Taken from webbrowser-app */
    ProgressBar {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: units.dp(3)
        showProgressPercentage: false
        visible: root.loading
        value: root.loadProgress / 100
    }
}
