/* The MIT License (MIT)
 *
 * Copyright (c) 2016 Jean Gressmann <jean@0x42.de>
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
import Sailfish.Pickers 1.0
import org.duckdns.jgressmann 1.0

import ".."


Page {
    id: root

    Component.onCompleted: {
        console.debug("format=" + settingDefaultFormat.value)
        console.debug("directory=" + settingDefaultDirectory.value)
        console.debug("fileName=" + settingDefaultFileName.value)
        _propageFileName()
    }

    Component.onDestruction: {
        settings.sync()
    }

    function _propageFileName() {
        console.debug("_propageFileName")
        if ("{title}" === settingDefaultFileName.value) {
            fileNameComboBox.currentIndex = 0
        } else if ("{id}" === settingDefaultFileName.value) {
            fileNameComboBox.currentIndex = 1
        } else {
            fileNameComboBox.currentIndex = 2
        }
    }

    function _getDirectory(str) {
        var lastSlashIndex = str.lastIndexOf("/")
        if (lastSlashIndex > 0) {
            str = str.substr(0, lastSlashIndex);
        }

        return str
    }

    function _updatesaveDirectoryComboBox() {
         // binding loop on currentIndex
        saveDirectoryComboBoxConnections.target = null
        if (settingDefaultDirectory.value === StandardPaths.download) {
            saveDirectoryComboBox.currentIndex = 0
        } else if (settingDefaultDirectory.value === StandardPaths.videos) {
            saveDirectoryComboBox.currentIndex = 1
        } else {
            saveDirectoryComboBox.currentIndex = 2
        }
        saveDirectoryComboBoxConnections.target = saveDirectoryComboBox
    }

    function _updateFileNameComboBox() {
         // binding loop on currentIndex
        fileNameConnections.target = null
        if (settingDefaultDirectory.value === StandardPaths.download) {
            saveDirectoryComboBox.currentIndex = 0
        } else if (settingDefaultDirectory.value === StandardPaths.videos) {
            saveDirectoryComboBox.currentIndex = 1
        } else {
            saveDirectoryComboBox.currentIndex = 2
        }
        fileNameConnections.target = fileNameComboBox
    }

    Connections {
        id: saveDirectoryComboBoxConnections
        target: saveDirectoryComboBox
        onCurrentIndexChanged: {
            switch (saveDirectoryComboBox.currentIndex) {
            case 0:
                settingDefaultDirectory.value = StandardPaths.download
                break
            case 1:
                settingDefaultDirectory.value = StandardPaths.videos
                break
            default:
                pageStack.push(filePickerPage)
                break
            }
        }
    }

//    Connections {
//        id: fileNameConnections
//        target: fileNameComboBox
//        onCurrentIndexChanged: {
//            switch (fileNameComboBox.currentIndex) {
//            case 0:
//                settingDefaultFileName.value = "{title}"
//                break
//            case 1:
//                settingDefaultFileName.value = "{id}"
//                break
//            }
//        }
//    }

    VisualItemModel {
        id: model

        SectionHeader {
            text: "Format"
        }

        FormatComboBox {
            excludeAskEveryTime: false
            format: settingDefaultFormat.value
            onFormatChanged: {
//                console.debug("save")
                settingDefaultFormat.value = format
            }
        }

        SectionHeader {
            text: "Location"
        }

        Column {
            spacing: Theme.paddingSmall
            width: parent.width

            ComboBox {
                id: saveDirectoryComboBox
                width: parent.width
                label: "Directory"
                menu: ContextMenu {
                    MenuItem { text: "Downloads" }
                    MenuItem { text: "Videos" }
                    MenuItem { text: "Custom" }
                }

                Component.onCompleted: _updatesaveDirectoryComboBox()
            }

            TextField {
                id: directoryTextField
                width: parent.width
                text: settingDefaultDirectory.value
                onTextChanged: _updatesaveDirectoryComboBox()
                label: "VODs are saved here"
                placeholderText: "Default directory to save VODs"
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
//                readOnly: true
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Pick directory"
                onClicked: pageStack.push(filePickerPage)
            }

            Component {
                id: filePickerPage
                FilePickerPage {
                    //nameFilters: [ '*.pdf', '*.doc' ]
                    nameFilters: []
                    onSelectedContentPropertiesChanged: {
                        settingDefaultDirectory.value = _getDirectory(selectedContentProperties.filePath)
                    }
                }
            }

            Item {
                height: Theme.paddingLarge
                width: 10
            }
        }

        Column {
            width: parent.width
            spacing: Theme.paddingSmall

            ComboBox {
                id: fileNameComboBox
                width: parent.width
                label: "File name"
                menu: ContextMenu {
                    MenuItem { text: "Title" }
                    MenuItem { text: "Video id" }
                    MenuItem { text: "Custom" }
                }

//                Component.onCompleted: _updateFileNameComboBox()

                onCurrentIndexChanged: {
                    console.debug("onCurrentIndexChanged " + currentIndex)
                    switch (currentIndex) {
                    case 0:
                        settingDefaultFileName.value = "{title}"
                        break
                    case 1:
                        settingDefaultFileName.value = "{id}"
                        break
                    }
                }
            }

            TextField {
                id: fileNameTextField
                width: parent.width
                text: settingDefaultFileName.value
                onTextChanged: {
                    settingDefaultFileName.value = text
                    _propageFileName()
                }
                label: "File name template for VODs"
                placeholderText: "File name template for VODs"
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
//                readOnly: true
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width-2*x
//                width: parent.width
                text: "Tags {title} {id} {formatid} will be substituded from the VOD's meta data."
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeTiny
                color: Theme.highlightColor
            }
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentWidth: parent.width

        VerticalScrollDecorator {}

        SilicaListView {
            id: listView
            anchors.fill: parent
            model: model
            header: PageHeader {
                title: "Settings"
            }
        }

    }
}

