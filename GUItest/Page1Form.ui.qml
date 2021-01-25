import QtQuick 2.4
import QtQuick.Controls.Basic.impl 6.0

Item {
    width: 400
    height: 400

    Row {
        id: row
        anchors.fill: parent
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0

        Column {
            id: column
            y: 100
            width: 200
            height: 300
        }

        Column {
            id: column1
            y: 100
            width: 200
            height: 300
        }
    }
}
