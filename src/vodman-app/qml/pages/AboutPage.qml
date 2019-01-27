/* The MIT License (MIT)
 *
 * Copyright (c) 2018, 2019 Jean Gressmann <jean@0x42.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.duckdns.jgressmann 1.0

Page {
    id: root


    SilicaFlickable {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: column.height

        VerticalScrollDecorator {}

        Column {
            id: column
            width: parent.width

            PageHeader {
                //% "About Vodman"
                title: qsTrId("about-page-header").arg(App.displayName)
            }


            Column {
                spacing: Theme.paddingLarge
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x



                Column {
                    spacing: Theme.paddingSmall
                    width: parent.width

                    Image {
                        source: "/usr/share/icons/hicolor/128x128/apps/harbour-vodman.png"
                        anchors.horizontalCenter: parent.horizontalCenter
                        fillMode: Image.PreserveAspectFit
                        width: Theme.iconSizeLarge
                        height: Theme.iconSizeLarge
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
    //                            console.debug("triggered")
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
                        //% "%1 %2"
                        text: qsTrId("about-version-text").arg(App.displayName).arg(App.version)
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: Theme.fontSizeLarge
                        color: Theme.highlightColor
                    }

                    Label {
                        text: "youtube-dl " + App.ytdlVersion
                        anchors.horizontalCenter: parent.horizontalCenter
                        font.pixelSize: Theme.fontSizeMedium
                        color: Theme.highlightColor
                    }

                    Button {
                        text: "Disable debugging"
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible: debugApp.value
                        onClicked: debugApp.value = false
                     }
                }

                SectionHeader {
                    //% "Description"
                    text: qsTrId("about-description-header")
                }

                Label {
                    width: parent.width
                    //% "%1 is a versatile tool..."
                    text: qsTrId("about-description-text").arg(App.displayName)
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.highlightColor
                    textFormat: TextEdit.RichText
                    onLinkActivated: Qt.openUrlExternally(link)
                }

                SectionHeader {
                    //% "Licensing"
                    text: qsTrId("about-licensing-header")
                }

                Label {
                    width: parent.width
                    //% "Copyright (c) 2018, 2019 Jean Gressmann..."
                    text: qsTrId("about-licensing-text").arg(App.displayName)
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.highlightColor
                    textFormat: TextEdit.RichText
                    onLinkActivated: Qt.openUrlExternally(link)
                }

                SectionHeader {
                    //% "Sources"
                    text: qsTrId("about-sources-header")
                }

                Label {
                    width: parent.width
                    //% "Sources are on Github"
                    text: qsTrId("about-sources-text")
                    wrapMode: Text.WordWrap
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.highlightColor
                    textFormat: TextEdit.RichText
                    onLinkActivated: Qt.openUrlExternally(link)
                }

                SectionHeader {
                    //% "Translations"
                    text: qsTrId("about-translations-header")
                }

                Column {
                    width: parent.width
                    spacing: Theme.paddingMedium

                    DetailItem {
                        //% "names of translators"
                        label: qsTrId("about-translations-english-label")
                        //% "English"
                        value: qsTrId("about-translations-english-value")
                    }

                    DetailItem {
                        //% "names of translators"
                        label: qsTrId("about-translations-german-label")
                        //% "German"
                        value: qsTrId("about-translations-german-value")
                    }

                    DetailItem {
                        //% "names of translators"
                        label: qsTrId("about-translations-swedish-label")
                        //% "Swedish"
                        value: qsTrId("about-translations-swedish-value")
                    }
                }
            }
        }
    }
}

