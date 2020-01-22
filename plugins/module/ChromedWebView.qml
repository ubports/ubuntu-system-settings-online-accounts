import QtQuick 2.9
import Ubuntu.Components 1.3
import Morph.Web 0.1

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
