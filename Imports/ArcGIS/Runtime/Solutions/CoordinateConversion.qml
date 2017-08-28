import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import Esri.ArcGISRuntime.Solutions 1.0

Rectangle {
    id: coordinateConversionWindow
    height: 800
    width: 600

    property bool getFromMapMode: false
    property int buttonWidth: 100
    property int spacingVal: 5

    CoordinateConversionController {
        id: controller
        objectName: "coordinateConversionController"
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 10

        Column {
            id: column
            anchors.margins: 30
            spacing: spacingVal * 2

            Row {
                spacing: spacingVal

                ComboBox {
                    id: notationTypeCombo
                    width: buttonWidth
                    model: CoordinateConversionOptions.coordinateTypeNames
                    onCurrentIndexChanged: {
                        controller.inputMode = CoordinateConversionOptions.stringToCoordinateType(currentText);
                    }
                }

                TextField {
                    id: inputNotation
                    width: buttonWidth * 3
                }

                Button {
                    text: "Convert"
                    enabled: inputNotation.text !== ""
                    onClicked: {
                        controller.convertNotation(inputNotation.text);
                    }
                }
            }

            Repeater {
                model: controller ? controller.results : null
                delegate:
                    Row {
                    spacing: spacingVal

                    Label {
                        width: buttonWidth
                        text: name
                    }

                    TextField {
                        id: notationBox
                        text: notation
                        width: buttonWidth * 3
                    }

                    Button {
                        text: "Copy to clipboard"
                        onClicked: {
                            controller.copyToClipboard(notationBox.text);
                            notationTypeCombo.currentIndex = notationTypeCombo.model.indexOf(CoordinateConversionOptions.coordinateTypeToString(coordinateType));
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: scrollView.right
            topMargin: 5
            bottomMargin: 5
            leftMargin: 10
            rightMargin: 30
        }
        height: 30

        CheckBox {
            id: getFromMap
            anchors {
                left: parent.left
                bottom: parent.bottom
                top: parent.top
            }

            text: "Get From Map"

            onCheckedChanged: {
                controller.runConversion = checked;
            }
        }

        Button {
            id: close
            anchors {
                right: parent.right
                bottom: parent.bottom
                top: parent.top
                rightMargin: 25
            }

            text: "Close"

            onClicked: {
                coordinateConversionWindow.visible = false;
            }
        }
    }
}
