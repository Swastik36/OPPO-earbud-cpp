import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    width: 440
    height: 680
    visible: true
    title: "OPPO Enco Buds3 Pro Companion"
    color: "#0F0F13"

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width - 40
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 20

            Item { height: 10 }

            // Header Title
            Text {
                text: "OPPO Enco Buds3 Pro"
                color: "white"
                font.pixelSize: 24
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            // Connection Status Subtitle
            Text {
                text: controller ? controller.statusText : "Disconnected"
                color: (controller && controller.isConnected) ? "#4CAF50" : "#888888"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignHCenter
            }

            // Connection Card (When Disconnected)
            Rectangle {
                visible: controller ? !controller.isConnected : true
                Layout.fillWidth: true
                height: 140
                radius: 20
                color: "#1AFFFFFF"
                border.color: "#33FFFFFF"
                border.width: 1

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12

                    TextField {
                        id: macInput
                        text: "60:55:56:22:49:A0"
                        placeholderText: "Bluetooth MAC Address"
                        Layout.preferredWidth: 260
                        horizontalAlignment: Text.AlignHCenter
                        color: "white"
                        background: Rectangle {
                            color: "#22FFFFFF"
                            radius: 10
                            border.color: "#44FFFFFF"
                        }
                    }

                    Button {
                        text: "Connect to Earbuds"
                        Layout.alignment: Qt.AlignHCenter
                        onClicked: if (controller) controller.connectDevice(macInput.text)
                    }
                }
            }

            // Glassmorphic Hero Battery Card
            Rectangle {
                visible: controller ? controller.isConnected : false
                Layout.fillWidth: true
                height: 140
                radius: 20
                color: "#1AFFFFFF"
                border.color: "#33FFFFFF"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 20

                    // Left Earbud
                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter
                        Text { text: "LEFT"; color: "#888888"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                        Text {
                            text: (controller ? controller.leftBattery : 0) + "%" + ((controller && controller.leftCharging) ? " ⚡" : "")
                            color: "#4CAF50"
                            font.pixelSize: 22
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }

                    // Right Earbud
                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter
                        Text { text: "RIGHT"; color: "#888888"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                        Text {
                            text: (controller ? controller.rightBattery : 0) + "%" + ((controller && controller.rightCharging) ? " ⚡" : "")
                            color: "#4CAF50"
                            font.pixelSize: 22
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }

                    // Charging Case
                    ColumnLayout {
                        Layout.alignment: Qt.AlignHCenter
                        Text { text: "CASE"; color: "#888888"; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                        Text {
                            text: ((controller && controller.caseBattery >= 0) ? controller.caseBattery + "%" : "--") + ((controller && controller.caseCharging) ? " ⚡" : "")
                            color: "white"
                            font.pixelSize: 22
                            font.bold: true
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            // Glassmorphic Sound Master EQ Card
            Rectangle {
                visible: controller ? controller.isConnected : false
                Layout.fillWidth: true
                height: 190
                radius: 20
                color: "#1AFFFFFF"
                border.color: "#33FFFFFF"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 10

                    Text { text: "Sound Master EQ"; color: "white"; font.pixelSize: 18; font.bold: true }

                    RadioButton {
                        text: "Original Sound"
                        checked: controller ? (controller.currentEQ === 0) : true
                        onClicked: if (controller) controller.setEQ(0)
                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; leftPadding: 30 }
                    }
                    RadioButton {
                        text: "Clear Vocals"
                        checked: controller ? (controller.currentEQ === 1) : false
                        onClicked: if (controller) controller.setEQ(1)
                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; leftPadding: 30 }
                    }
                    RadioButton {
                        text: "Bass Boost"
                        checked: controller ? (controller.currentEQ === 2) : false
                        onClicked: if (controller) controller.setEQ(2)
                        contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 14; leftPadding: 30 }
                    }
                }
            }

            // Glassmorphic Game Mode Switch Card
            Rectangle {
                visible: controller ? controller.isConnected : false
                Layout.fillWidth: true
                height: 80
                radius: 20
                color: "#1AFFFFFF"
                border.color: "#33FFFFFF"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 20

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Low-Latency Game Mode"; color: "white"; font.pixelSize: 16; font.bold: true }
                        Text { text: "Reduces audio latency during gaming"; color: "#888888"; font.pixelSize: 12 }
                    }

                    Switch {
                        checked: controller ? controller.gameMode : false
                        onCheckedChanged: {
                            if (controller && checked !== controller.gameMode) {
                                controller.setGameMode(checked)
                            }
                        }
                    }
                }
            }

            // Disconnect Button
            Button {
                visible: controller ? controller.isConnected : false
                text: "Disconnect"
                Layout.alignment: Qt.AlignHCenter
                onClicked: if (controller) controller.disconnectDevice()
            }

            Item { height: 20 }
        }
    }
}
