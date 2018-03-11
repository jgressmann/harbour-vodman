import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

CoverBackground {

//    Image {
//        anchors.fill: parent
//        source: "image://theme/icon-m-device-download"
//        fillMode: Image.Pad
//    }

    CoverPlaceholder {
        text: "No downloads at present"
        visible: listView.count === 0

//        icon.source: "/usr/share/icons/hicolor/128x128/apps/harbour-vodman.png"
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: vodDownloadModel

        delegate: ListItem {
            id: listItem
            contentHeight: Theme.itemSizeSmall
            width: parent.width - 2 * Theme.paddingSmall
            x: Theme.paddingSmall

            ProgressOverlay {
                anchors.fill: parent
                progress: download.data.progress
                inverse: _inverse

                Image {
                    id: thumnail
                    source: download.data.description.thumbnailUrl
                    width: parent.height
                    height: parent.height
                    fillMode: Image.PreserveAspectFit
//                    asynchronous: true
                }

                Item {
                    height: parent.height
                    width: parent.width - thumnail.width
                    x: thumnail.width

                    Label {
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        text: (download.data.progress * 100).toFixed(0) + "%"
                        font.pixelSize: Theme.fontSizeSmall
                        truncationMode: TruncationMode.Fade
                    }
                }
            }
        }

//        ViewPlaceholder {
//            enabled: listView.count === 0
//            text: "No downloads at present"
//        }
    }

    CoverActionList {
        id: coverAction
        enabled: listView.count > 0

//        CoverAction {
//            iconSource: "image://theme/icon-cover-next"
//        }

        CoverAction {
            iconSource: "image://theme/icon-cover-cancel"
            onTriggered: vodDownloadModel.cancelDownloads(false)
        }
    }


}

