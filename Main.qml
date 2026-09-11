pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root

    required property MarketInfoSummaryModel marketInfoSummary

    // Window
    readonly property int windowWidth: 500
    readonly property int windowHeight: 465
    readonly property int windowMinimumWidth: 460
    readonly property int windowMinimumHeight: 465

    // Layout
    readonly property int outerMargin: 14
    readonly property int sectionSpacing: 10
    readonly property int horizontalPadding: 14
    readonly property int labelRightPadding: 12
    readonly property int valueColumnWidth: 128
    readonly property int valueTextWidth: valueColumnWidth - 2 * horizontalPadding

    // Dimensions
    readonly property int tableHeaderHeight: 32
    readonly property int sectionHeaderHeight: 32
    readonly property int summaryRowHeight: 33
    readonly property int separatorWidth: 1
    readonly property int panelBorderWidth: 1
    readonly property int panelRadius: 4

    // Typography
    readonly property int titleFontSize: 20
    readonly property int bodyFontSize: 13

    // Colors
    readonly property color windowColor: "#eef1f4"
    readonly property color borderColor: "#c9ced6"
    readonly property color headerColor: "#1769aa"
    readonly property color headerDividerColor: Qt.lighter(headerColor, 1.25)
    readonly property color headerTextColor: "#ffffff"
    readonly property color primaryTextColor: "#1f2933"
    readonly property color secondaryTextColor: "#52606d"
    readonly property color panelColor: "#ffffff"
    readonly property color alternateRowColor: "#f7f9fb"
    readonly property color sectionHeaderColor: "#e8edf3"

    // Text
    readonly property string windowTitleText: qsTr("Market Information Summary")
    readonly property string messageHeaderText: qsTr("Message")
    readonly property string countHeaderText: qsTr("Count")
    readonly property string heartbeatHeaderText: qsTr("Heartbeat")
    readonly property string serverHeartbeatText: qsTr("Last Server Heartbeat")
    readonly property string clientHeartbeatText: qsTr("Last Client Heartbeat")
    readonly property string emptyHeartbeatText: qsTr("--:--:-- WIB")

    visible: true
    width: windowWidth
    height: windowHeight
    minimumWidth: windowMinimumWidth
    minimumHeight: windowMinimumHeight
    title: windowTitleText
    color: windowColor

    component SummaryRow: Rectangle {
        id: row

        required property string rowLabel
        required property var rowValue
        property bool alternate: false
        property string emptyValueText: ""

        implicitHeight: root.summaryRowHeight
        color: alternate ? root.alternateRowColor : root.panelColor

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: root.separatorWidth
            color: root.borderColor
        }

        Rectangle {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: root.valueColumnWidth
            width: root.separatorWidth
            color: root.borderColor
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: root.horizontalPadding
            anchors.right: parent.right
            anchors.rightMargin: root.valueColumnWidth + root.labelRightPadding
            anchors.verticalCenter: parent.verticalCenter
            text: row.rowLabel
            color: root.primaryTextColor
            font.pixelSize: root.bodyFontSize
            elide: Text.ElideRight
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: root.horizontalPadding
            anchors.verticalCenter: parent.verticalCenter
            width: root.valueTextWidth
            horizontalAlignment: Text.AlignRight
            text: row.rowValue === "" ? row.emptyValueText : row.rowValue
            color: row.rowValue === "" ? root.secondaryTextColor : root.primaryTextColor
            font.pixelSize: root.bodyFontSize
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.outerMargin
        spacing: root.sectionSpacing

        Text {
            Layout.fillWidth: true
            text: root.windowTitleText
            color: root.primaryTextColor
            font.pixelSize: root.titleFontSize
            font.weight: Font.DemiBold
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: messageLayout.implicitHeight
            color: root.panelColor
            border.width: root.panelBorderWidth
            border.color: root.borderColor
            radius: root.panelRadius
            clip: true

            ColumnLayout {
                id: messageLayout
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: root.tableHeaderHeight
                    color: root.headerColor

                    Rectangle {
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        anchors.rightMargin: root.valueColumnWidth
                        width: root.separatorWidth
                        color: root.headerDividerColor
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: root.horizontalPadding
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.messageHeaderText
                        color: root.headerTextColor
                        font.pixelSize: root.bodyFontSize
                        font.weight: Font.DemiBold
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: root.horizontalPadding
                        anchors.verticalCenter: parent.verticalCenter
                        width: root.valueTextWidth
                        horizontalAlignment: Text.AlignRight
                        text: root.countHeaderText
                        color: root.headerTextColor
                        font.pixelSize: root.bodyFontSize
                        font.weight: Font.DemiBold
                    }
                }

                Repeater {
                    model: root.marketInfoSummary

                    delegate: SummaryRow {
                        required property int index
                        required property string label
                        required property var count

                        Layout.fillWidth: true
                        rowLabel: label
                        rowValue: count
                        alternate: index % 2 === 1
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: heartbeatLayout.implicitHeight
            color: root.panelColor
            border.width: root.panelBorderWidth
            border.color: root.borderColor
            radius: root.panelRadius
            clip: true

            ColumnLayout {
                id: heartbeatLayout
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: root.sectionHeaderHeight
                    color: root.sectionHeaderColor

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: root.horizontalPadding
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.heartbeatHeaderText
                        color: root.primaryTextColor
                        font.pixelSize: root.bodyFontSize
                        font.weight: Font.DemiBold
                    }
                }

                SummaryRow {
                    Layout.fillWidth: true
                    rowLabel: root.serverHeartbeatText
                    rowValue: root.marketInfoSummary.lastServerHeartbeat
                    emptyValueText: root.emptyHeartbeatText
                }

                SummaryRow {
                    Layout.fillWidth: true
                    rowLabel: root.clientHeartbeatText
                    rowValue: root.marketInfoSummary.lastClientHeartbeat
                    emptyValueText: root.emptyHeartbeatText
                    alternate: true
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}