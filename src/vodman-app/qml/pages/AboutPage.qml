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
                    source: "/usr/share/icons/hicolor/86x86/apps/harbour-vodman.png"
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Label {
                    text: "Vodman 1.0.0"
                    anchors.horizontalCenter: parent.horizontalCenter
                    font.pixelSize: Theme.fontSizeLarge
//                    font.bold: true
                }
            }

            SectionHeader {
                text: "Description"
            }

            TextArea {
                 width: parent.width
                 text:
"Vodman is a versatile tool for downloading vidoes on demand (VODs) to your device from " +
"popular sites such as <a href=\"https://www.youtube.com/\">Youtube</a> and <a href=\"https://www.twitch.tv/\">Twitch</a>."
                 readOnly: true
                 font.pixelSize: Theme.fontSizeTiny
                 color: Theme.primaryColor
//                 TextField.text: TextEdit.RichText
            }

            SectionHeader {
                text: "Licensing"
            }

            TextArea {
                 width: parent.width
                 text: "Copyright (c) 2018 by Jean Gressmann.

Vodman is available under the <a href=\"https://opensource.org/licenses/MIT\">MIT</a> license. " +
"Vodman uses youtube-dl which is in the <a href=\"http://unlicense.org/\">public domain</a>."
                 readOnly: true
                 font.pixelSize: Theme.fontSizeTiny
                 color: Theme.primaryColor
            }
        }
    }
}

