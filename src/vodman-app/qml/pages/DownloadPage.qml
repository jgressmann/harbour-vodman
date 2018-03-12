import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Notifications 1.0
import org.duckdns.jgressmann 1.0
import ".."


Page {
    id: root
    property bool _inverse: false

    function targetWidth(format) {
        switch (format) {
        case VM.VM_240p:
            return 240
        case VM.VM_360p:
            return 360
        case VM.VM_720p:
            return 720
        default:
            return 1080
        }
    }

    function metaDataDownloadSucceeded(token, vod) {
        var formatId = defaultFormat.value
//        console.debug("format=" + formatId)
        if (VM.VM_Any === formatId) {
//            console.debug("open select format dialog")
            var dialog = pageStack.push(Qt.resolvedUrl("SelectFormatDialog.qml"))
            dialog.accepted.connect(function() {
                metaDataDownloadSucceededEx(token, vod, dialog.format)
            })
            return
        }

        metaDataDownloadSucceededEx(token, vod, formatId)
    }

    function metaDataDownloadSucceededEx(token, vod, formatId) {
//        console.debug("token=" + token + ", vod=" + vod)
        console.debug("formatId=" + formatId)
        console.debug(".description=" + vod.description)
        console.debug("#formats=" + vod.formats)
        for (var i = 0; i < vod.formats; ++i) {
//            _vodFormat = vod.format(i)
            var f = vod.format(i)
//            console.debug(i + " " + f)
            console.debug(i + " " + f.width + "x" + f.height)
        }

        var formatIndex = -1
        if (VM.VM_Smallest === formatId) {
            var best = vod.format(0)
            for (var i = 1; i < vod.formats; ++i) {
                var f = vod.format(i)
                if (f.height < best.height) {
                    best = f;
                    formatIndex = i;
                }
            }
        } else if (VM.VM_Largest === formatId) {
            var best = vod.format(0)
            for (var i = 1; i < vod.formats; ++i) {
                var f = vod.format(i)
                if (f.height > best.height) {
                    best = f;
                    formatIndex = i;
                }
            }
        } else {
            // try to find exact match
            for (var i = 0; i < vod.formats; ++i) {
                var f = vod.format(i)
                if (f.format === defaultFormat) {
                    formatIndex = i
                    break
                }
            }

            if (formatIndex === -1) {
                var target = targetWidth(formatId)
                var bestdelta = Math.abs(vod.format(0).height - target)
                for (var i = 1; i < vod.formats; ++i) {
                    var f = vod.format(i)
                    var delta = Math.abs(f.width - target)
                    if (delta < bestdelta) {
                        bestdelta = delta;
                        formatIndex = i;
                    }
                }
            }
        }

        var path = defaultDirectory.value
        if (!path) {
            path = StandardPaths.download
        }

        var format = vod.format(formatIndex)
        path = path + "/" + vod.description.id + "_" + format.id + "." + format.fileExtension

        console.debug("format=" + format)
        console.debug("path=" + path)
        vodDownloadModel.startDownloadVod(token, vod, formatIndex, path)
    }

    function downloadFailed(url, error, filePath) {
        console.debug("url=" + url + ", error=" + error, ", path=" + filePath)
        switch (error) {
        case VM.VM_ErrorCanceled:
            break;
        default:
            notification.publish()
            break;
        }
    }

    function downloadSucceeded(download) {
        console.debug("download=" + download)

    }

    Component.onCompleted: {
        vodDownloadModel.metaDataDownloadSucceeded.connect(metaDataDownloadSucceeded)
        vodDownloadModel.downloadFailed.connect(downloadFailed)
        vodDownloadModel.downloadSucceeded.connect(downloadSucceeded)
    }

    Notification {
         id: notification
         category: "x-nemo.example"
         summary: "Notification summary"
         body: "Notification body"
         onClicked: console.log("Clicked")
    }

    Notification {
        id: successNotification
        category: "x-nemo.example"
        appName: "Example App"
        appIcon: "/usr/share/example-app/icon-l-application"
        summary: "Notification summary"
        body: "Notification body"
        previewSummary: "Notification preview summary"
        previewBody: "Notification preview body"
        onClicked: console.log("Clicked")
    }



    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                text: qsTr("Download small video")
                visible: debugApp.value
                enabled: vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.startDownloadMetaData("https://www.youtube.com/watch?v=7t-l0q_v4D8")
            }

            MenuItem {
                text: qsTr("Download large video")
                visible: debugApp.value
                enabled: vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.startDownloadMetaData("https://www.twitch.tv/videos/161472611?t=07h49m09s")
            }

            MenuItem {
                text: qsTr("Download medium video")
                visible: debugApp.value
                enabled: vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.startDownloadMetaData("https://www.youtube.com/watch?v=KMAqSLWhH5w")
            }

            MenuItem {
                text: qsTr("reddit")
                visible: debugApp.value
                enabled: vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.startDownloadMetaData("https://www.reddit.com")
            }

//            MenuItem {
//                text: qsTr("Reverse progress direction")
//                onClicked: {
//                    _inverse = !_inverse
//                }
//            }

//            MenuItem {
//                text: qsTr("Pick format dialog")
//                onClicked: pageStack.push(Qt.resolvedUrl("SelectFormatDialog.qml"))
//            }

            MenuItem {
                text: "Cancel all downloads"
                visible: listView.count > 0
                onClicked: {
                      remorse.execute(
                          "Stopping all downlods",
                          function() { vodDownloadModel.cancelDownloads(false) })
                }
            }

            MenuItem {
                text: "Cancel all downloads and delete files"
                visible: listView.count > 0
                onClicked: {
                      remorse.execute(
                          "Purging all downlods",
                          function() { vodDownloadModel.cancelDownloads(true) })
                }
            }

            MenuItem {
                text: qsTr("Download from clipboard")
                enabled: Clipboard.hasText && vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.startDownloadMetaData(Clipboard.text)
            }
        }


        PushUpMenu {
            MenuItem {
                text: "Settings"
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }

            MenuItem {
                text: "About Vodman"
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }
        }

        //contentWidth: parent.width - 2 * Theme.paddingMedium
        contentWidth: parent.width


        VerticalScrollDecorator {}

        SilicaListView {
            id: listView
            visible: true
            anchors.fill: parent
            model: vodDownloadModel
            header: PageHeader {
                title: "Downloads"

                BusyIndicator {
                    running: !vodDownloadModel.canStartDownload
                    size: BusyIndicatorSize.Small
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            delegate: ListItem {
                id: listItem
                menu: contextMenu
                contentHeight: Theme.itemSizeHuge
                width: parent.width - 2 * x
                x: Theme.paddingMedium
                ListView.onRemove: animateRemoval(listItem)

                function cancelDownload(deleteFile) {
                    remorseAction(
                        "Stop downloading " + download.data.description.fullTitle,
                        function() { vodDownloadModel.cancelDownload(index, deleteFile) })
                }


                ProgressOverlay {
                    anchors.fill: parent
                    progress: download.data.progress
                    inverse: _inverse

                    Row {
                        spacing: Theme.paddingMedium
                        height: parent.height

                        Image {
                            id: thumnail
                            source: download.data.description.thumbnailUrl
                            width: parent.height
                            height: parent.height
                            fillMode: Image.PreserveAspectFit
                            // prevents the image from loading on device
                            //asynchronous: true
                        }

                        Column {
                            //y: height - parent.height * 0.5
                            width: listItem.width - x
                            anchors.verticalCenter: parent.verticalCenter

                            Label {
                                id: title
                                width: parent.width
                                text: download.data.description.fullTitle
                                font.pixelSize: Theme.fontSizeSmall
                                truncationMode: TruncationMode.Fade
                            }

                            Label {
                                width: parent.width
                                text: {
                                    var str = ""
                                    str += download.data.format.width
                                    str += "x"
                                    str += download.data.format.height
//                                    str += ", "
//                                    str += download.data.filePath

                                    return str
                                }
                                font.pixelSize: Theme.fontSizeTiny
                                color: Theme.secondaryColor
                                truncationMode: TruncationMode.Fade
                            }

                            Label {
                                width: parent.width
                                text: download.data.filePath
                                font.pixelSize: Theme.fontSizeTiny
                                color: Theme.secondaryColor
                                truncationMode: TruncationMode.Fade
                            }

//                            Label {
//                                width: parent.width
//                                text: download.data.description.webPageUrl
//                                font.pixelSize: Theme.fontSizeTiny
//                                color: Theme.secondaryColor
//                                truncationMode: TruncationMode.Fade
//                                textFormat: TextEdit.RichText
//                            }

                            LinkedLabel {
                                width: parent.width
                                plainText: download.data.description.webPageUrl
                                font.pixelSize: Theme.fontSizeTiny
                                color: Theme.secondaryColor
//                                truncationMode: TruncationMode.Fade
                            }
                        }

                    }
                }



                Component {
                    id: contextMenu
                    ContextMenu {
                        MenuItem {
                            text: "Cancel and delete file"
                            onClicked: cancelDownload(true)
                        }

                        MenuItem {
                            text: "Cancel"
                            onClicked: cancelDownload(false)
                        }

                        MenuItem {
                            text: "Play"
                            onClicked: Qt.openUrlExternally("file://" + download.data.filePath)
                        }

                        MenuItem {
                            text: "Open webpage"
                            onClicked: {
                                console.debug("opening: " + download.data.description.webPageUrl)
                                Qt.openUrlExternally(download.data.description.webPageUrl)
                            }
                        }
                    }
                }
            }

            ViewPlaceholder {
                enabled: listView.count === 0
                text: {
                    if (vodDownloadModel.canStartDownload) {
                        return "No downloads at present"
                    }
                    return "Downloading VOD metadata"
                }
            }
        }
    }
}

