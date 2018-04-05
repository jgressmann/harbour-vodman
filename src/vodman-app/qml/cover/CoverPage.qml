/* The MIT License (MIT)
 *
 * Copyright (c) 2018 Jean Gressmann <jean@0x42.de>
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
import ".."

CoverBackground {
    Item {
        width: 2 * parent.width
        height: 2 * parent.width
        transform: Rotation { origin.x: parent.width; origin.y: parent.width; angle: 45 }

        Item {
            anchors.fill: parent
            transform: Translate { x: -0.5*parent.width; y: 0.67*parent.width }

            Image {
                anchors.fill: parent
                source: "/usr/share/harbour-vodman/icons/tape.png"
                fillMode: Image.PreserveAspectFit
            }
        }
    }



    Label {
        x: Theme.paddingMedium
        y: Theme.paddingLarge
        height: 0.5 * parent.height - 2*y
        width: parent.width - 2 * x
        text: "No downloads at present"
        visible: listView.count === 0
        wrapMode: Text.WordWrap
        font.pixelSize: Theme.fontSizeMedium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }


    SilicaListView {
        id: listView
        anchors.fill: parent
        model: vodDownloadModel

        delegate: ListItem {
            id: listItem
            contentHeight: Theme.itemSizeSmall
            x: Theme.paddingSmall
            width: parent.width - 2 * x


            ProgressOverlay {
                anchors.fill: parent
                progress: download.data.progress

                Image {
                    id: thumnail
                    source: download.data.description.thumbnailUrl
                    width: parent.height
                    height: parent.height
                    fillMode: Image.PreserveAspectFit
//                    visible: state === Image.Ready
                    visible: progress >= 1 // state === Image.Ready doesn't work
//                    onStateChanged: {
//                        console.debug("state=" + state)
//                        switch (state) {
//                        case Image.Null:
//                            console.debug("null")
//                            break
//                        case Image.Loading:
//                            console.debug("loading")
//                            break
//                        case Image.Error:
//                            console.debug("error")
//                            break
//                        case Image.Ready:
//                            console.debug("ready")
//                            break
//                        }
//                    }

//                    onProgressChanged: {
//                        console.debug("progress="+progress)
//                    }

//                    visible: false
//                    asynchronous: true
                }

                Item {
                    height: parent.height
                    width: parent.width

                    Label {
                        anchors.left: parent.left
                        anchors.right: percentLabel.left
                        anchors.verticalCenter: parent.verticalCenter
                        text: download.data.description.fullTitle
                        font.pixelSize: Theme.fontSizeSmall
                        truncationMode: TruncationMode.Fade
                        visible: !thumnail.visible
                    }

                    Label {
                        id: percentLabel
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: (download.data.progress * 100).toFixed(0) + "%"
                        font.pixelSize: Theme.fontSizeSmall
                        truncationMode: TruncationMode.Fade
                    }
                }
            }
        }
    }

    CoverActionList {
        property bool clipBoardHasUrl: Clipboard.hasText && vodDownloadModel.isUrl(Clipboard.text)
        property bool hasDefaultVideoFormat: {
            if (vodDownloadModel.isOnBroadband && settingBroadbandDefaultFormat.value !== VM.VM_Any) {
                return true
            }

            if (vodDownloadModel.isOnMobile && settingMobileDefaultFormat.value !== VM.VM_Any) {
                return true
            }

            return false
        }

        id: coverAction
        enabled: hasDefaultVideoFormat &&
                 vodDownloadModel.canStartDownload &&
                 clipBoardHasUrl

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: vodDownloadModel.startDownloadMetaData(Clipboard.text)
        }
    }
}

