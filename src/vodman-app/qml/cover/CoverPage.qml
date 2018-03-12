import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

CoverBackground {

//    Image {
////        anchors.fill: parent
//        source: "/usr/share/harbour-vodman/icons/cover.svg"
//        fillMode: Image.PreserveAspectFit
//        y: 0.5 * parent.height
////        width: parent.width
////        height: 0.5 * parent.height
//        sourceSize.width: parent.width
//        sourceSize.height: 0.5 * parent.height
//    }

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
    }

    CoverActionList {
        id: coverAction
        enabled: listView.count > 0

        CoverAction {
            iconSource: "image://theme/icon-cover-cancel"
            onTriggered: vodDownloadModel.cancelDownloads(false)
        }
    }
}

