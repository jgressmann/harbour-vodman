import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import org.duckdns.jgressmann 1.0
import ".."


Page {
    id: root

    Component.onCompleted: {
//        console.debug("format=" + defaultFormat.value)
//        console.debug("directory=" + defaultDirectory.value)
    }

    Component.onDestruction: {
        settings.sync()
    }

    function getDirectory(str) {
        var lastSlashIndex = str.lastIndexOf("/")
        if (lastSlashIndex > 0) {
            str = str.substr(0, lastSlashIndex);
        }

        return str
    }

    function updateSaveDirectoryComboBox() {
         // binding loop on currentIndex
        saveDirectoryConnections.target = null
        if (defaultDirectory.value === StandardPaths.download) {
            saveDirectory.currentIndex = 0
        } else if (defaultDirectory.value === StandardPaths.videos) {
            saveDirectory.currentIndex = 1
        } else {
            saveDirectory.currentIndex = 2
        }
        saveDirectoryConnections.target = saveDirectory
    }

    Connections {
        id: saveDirectoryConnections
        target: saveDirectory
        onCurrentIndexChanged: {
            switch (saveDirectory.currentIndex) {
            case 0:
                defaultDirectory.value = StandardPaths.download
                break
            case 1:
                defaultDirectory.value = StandardPaths.videos
                break
            default:
                pageStack.push(filePickerPage)
                break
            }
        }
    }

    VisualItemModel {
        id: model

        SectionHeader {
            text: "Default"
        }

        FormatComboBox {
            excludeAskEveryTime: false
            format: defaultFormat.value
            onFormatChanged: {
                console.debug("save")
                defaultFormat.value = format
            }
        }

        ComboBox {
            id: saveDirectory
            width: parent.width
            label: "Save directory"
            menu: ContextMenu {
                MenuItem { text: "Downloads" }
                MenuItem { text: "Videos" }
                MenuItem { text: "Custom" }
            }

            Component.onCompleted: updateSaveDirectoryComboBox()
        }

        Column {
            width: parent.width

            TextField {
                id: directoryTextField
                width: parent.width
                text: defaultDirectory.value
                onTextChanged: updateSaveDirectoryComboBox()
                label: "VODs are saved here"
                placeholderText: "Default directory to save VODs"
                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
                readOnly: true
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
                        defaultDirectory.value = getDirectory(selectedContentProperties.filePath)
                    }
                }
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

