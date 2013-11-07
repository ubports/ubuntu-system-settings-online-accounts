import QtQuick 2.0

Rectangle {
    id: root

    property Item flickable: flickableItem
    signal finished

    width: 200
    height: 200

    Timer {
        interval: 50; running: true; repeat: false
        onTriggered: root.finished()
    }

    Flickable {
        id: flickableItem
    }
}
