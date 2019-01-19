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
import Sailfish.Pickers 1.0
import org.duckdns.jgressmann 1.0
import ".."


Page {
    id: root

    Component.onCompleted: {
//        console.debug("broadband format=" + settingBroadbandDefaultFormat.value)
//        console.debug("mobile format=" + settingMobileDefaultFormat.value)
//        console.debug("directory=" + settingDefaultDirectory.value)
//        console.debug("fileName=" + settingDefaultFileName.value)
        _propageFileName()
    }

    Component.onDestruction: {
        settings.sync()
    }

    function _propageFileName() {
//        console.debug("_propageFileName")
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

    function _updateSaveDirectoryComboBox() {
        settingDefaultDirectory.value = saveDirectoryTextField.text

        if (settingDefaultDirectory.value === StandardPaths.download) {
            saveDirectoryComboBox.currentIndex = 0
        } else if (settingDefaultDirectory.value === StandardPaths.videos) {
            saveDirectoryComboBox.currentIndex = 1
        } else {
            saveDirectoryComboBox.currentIndex = 2
        }
    }

    function _onSaveDirectoryTextFieldTextChanged() {
        saveDirectoryComboBoxConnections.target = null
        _updateSaveDirectoryComboBox()
        saveDirectoryComboBoxConnections.target = saveDirectoryComboBox
    }

    function _updateSaveDirectoryTextField() {
        switch (saveDirectoryComboBox.currentIndex) {
        case 0:
            settingDefaultDirectory.value = saveDirectoryTextField.text = StandardPaths.download
            break
        case 1:
            settingDefaultDirectory.value = saveDirectoryTextField.text = StandardPaths.videos
            break
        default:
            pageStack.push(filePickerPage)
            break
        }
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
            saveDirectoryTextFieldConnections.target = null
            _updateSaveDirectoryTextField()
            saveDirectoryTextFieldConnections.target = saveDirectoryTextField
        }
    }

    Connections {
        id: saveDirectoryTextFieldConnections
        target: saveDirectoryTextField
        onTextChanged: _onSaveDirectoryTextFieldTextChanged()
    }

    VisualItemModel {
        id: model

        SectionHeader {
            text: qsTr("Network")
        }

        ComboBox {
            id: bearerModeComboBox
            width: parent.width
            label: qsTr("Network connection type")
            menu: ContextMenu {
                MenuItem { text: qsTr("Autodetect") }
                MenuItem { text: qsTr("Broadband") }
                MenuItem { text: qsTr("Mobile") }
            }

            Component.onCompleted: currentIndex = settingBearerMode.value

            onCurrentIndexChanged: {
//                console.debug("bearer mode onCurrentIndexChanged " + currentIndex)
                settingBearerMode.value = currentIndex
            }
        }

        SectionHeader {
            text: qsTr("Format")
        }

        FormatComboBox {
            label: qsTr("Broadband")
            excludeAskEveryTime: false
            format: settingBroadbandDefaultFormat.value
            onFormatChanged: {
//                console.debug("save")
                settingBroadbandDefaultFormat.value = format
            }
        }

        FormatComboBox {
            label: qsTr("Mobile")
            excludeAskEveryTime: false
            format: settingMobileDefaultFormat.value
            onFormatChanged: {
//                console.debug("save")
                settingMobileDefaultFormat.value = format
            }
        }

        SectionHeader {
            text: qsTr("Location")
        }

        Column {
            spacing: Theme.paddingSmall
            width: parent.width

            ComboBox {
                id: saveDirectoryComboBox
                width: parent.width
                label: qsTr("Directory")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Downloads") }
                    MenuItem { text: qsTr("Videos") }
                    MenuItem { text: qsTr("Custom") }
                }
            }

            TextField {
                id: saveDirectoryTextField
                width: parent.width
                text: settingDefaultDirectory.value
                label: qsTr("VODs are saved here", "label")
                placeholderText: qsTr("Default directory to save VODs", "placeholder")
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false

                Component.onCompleted: {
                    // combobox initially has index 0 which may be wrong
                    _onSaveDirectoryTextFieldTextChanged()
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Pick directory")
                onClicked: pageStack.push(filePickerPage)
            }

            Component {
                id: filePickerPage
                FilePickerPage {
                    //nameFilters: [ '*.pdf', '*.doc' ]
                    nameFilters: []
                    onSelectedContentPropertiesChanged: {
                        settingDefaultDirectory.value = saveDirectoryTextField.text = _getDirectory(selectedContentProperties.filePath)
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
                    MenuItem { text: qsTr("Title") }
                    MenuItem { text: qsTr("Video id") }
                    MenuItem { text: qsTr("Custom") }
                }

                onCurrentIndexChanged: {
//                    console.debug("onCurrentIndexChanged " + currentIndex)
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
                label: qsTr("File name template for VODs", "label")
                placeholderText: qsTr("File name template for VODs", "placeholder")
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
//                readOnly: true
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width-2*x
//                width: parent.width
                text: qsTr("Tags {title} {id} {formatid} will be substituded from the VOD's meta data.")
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
                title: qsTr("Settings")
            }
        }

    }
}

