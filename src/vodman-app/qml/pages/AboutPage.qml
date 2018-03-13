import QtQuick 2.0
import Sailfish.Silica 1.0


Page {
    id: root


    SilicaFlickable {
        anchors.fill: parent
        contentWidth: parent.width

        VerticalScrollDecorator {}

        Column {
            spacing: Theme.paddingLarge
            x: Theme.paddingMedium
            width: parent.width - 2*x
            PageHeader { title: "About Vodman" }

            Column {
                spacing: Theme.paddingSmall
                width: parent.width

                Image {
                    //source: "/usr/share/harbour-vodman/icons/icon.svg"
                    source: "/usr/share/icons/hicolor/128x128/apps/harbour-vodman.png"
                    anchors.horizontalCenter: parent.horizontalCenter
                    fillMode: Image.PreserveAspectFit
//                    sourceSize.width: 512
//                    sourceSize.height: 512

                    MouseArea {
                        id: debugEnabler
                        property int clicks: 0
                        anchors.fill: parent

                        onClicked: {
                            timer.running = true
                            clicks = clicks + 1
                        }

                        function timerDone() {
                            console.debug("triggered")
                            if (clicks >= 10) {
                                debugApp.value = true
                            }

                            clicks = 0
                        }

                        Timer {
                            id: timer
                            interval: 3000; running: false; repeat: false
                            onTriggered: debugEnabler.timerDone()
                        }
                    }
                }

                Label {
                    text: "Vodman 1.0.0"
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: Theme.fontSizeLarge
                    color: Theme.highlightColor
//                    font.bold: true
                }

                Label {
                    text: "youtube-dl 2017.11.06"
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.highlightColor
//                    font.bold: true
                }

                Button {
                    text: "Disable debugging"
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: debugApp.value
                    onClicked: {
                        debugApp.value = false
                    }
                 }
            }

            SectionHeader {
                text: "Description"
            }

            Label {
                x: Theme.horizontalPageMargin
                 width: parent.width-2*x
                 text:
"Vodman is a versatile tool for downloading vidoes on demand (VODs) to your device from " +
"popular sites such as <a href=\"https://www.youtube.com/\">Youtube</a> or <a href=\"https://www.twitch.tv/\">Twitch</a>."
                 wrapMode: Text.WordWrap
                 font.pixelSize: Theme.fontSizeExtraSmall
                 color: Theme.highlightColor
                 textFormat: TextEdit.RichText
//                 TextField.text:
            }

            SectionHeader {
                text: "Licensing"
            }

            Label {
                x: Theme.horizontalPageMargin
                 width: parent.width-2*x
                 text: "Copyright (c) 2018 by Jean Gressmann.

Vodman is available under the <a href=\"https://opensource.org/licenses/MIT\">MIT</a> license. " +
"Vodman uses youtube-dl which is in the <a href=\"http://unlicense.org/\">public domain</a>."
                 wrapMode: Text.WordWrap
                 font.pixelSize: Theme.fontSizeExtraSmall
                 color: Theme.highlightColor
                 textFormat: TextEdit.RichText
            }
        }
    }
}

