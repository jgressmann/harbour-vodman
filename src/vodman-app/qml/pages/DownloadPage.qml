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
import Nemo.Notifications 1.0
import Nemo.Configuration 1.0
import Nemo.DBus 2.0
import org.duckdns.jgressmann 1.0

import ".."


Page {
    id: page

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
//        console.debug("token=" + token + ", vod=" + vod)
        console.debug(".description=" + vod.description)
        console.debug("#formats=" + vod.formats)
        for (var i = 0; i < vod.formats; ++i) {
//            _vodFormat = vod.format(i)
            var f = vod.format(i)
//            console.debug(i + " " + f)
            console.debug(i + " " + f.width + "x" + f.height + " format " + f.format)
        }

        var formatId
        switch (settingBearerMode.value) {
        case Global.bearerModeBroadband:
            console.debug("force broadband format selection")
            formatId = settingBroadbandDefaultFormat.value
            break
        case Global.bearerModeMobile:
            console.debug("force mobile format selection")
            formatId = settingMobileDefaultFormat.value
            break
        default:
            if (vodDownloadModel.isOnBroadband) {
                console.debug("use broadband format selection")
                formatId = settingBroadbandDefaultFormat.value
            } else if (vodDownloadModel.isOnMobile) {
                console.debug("use mobile format selection")
                formatId = settingMobileDefaultFormat.value
            } else {
                console.debug("unknown bearer using mobile default format")
                formatId = settingMobileDefaultFormat.value
            }
            break
        }

        console.debug("format=" + formatId)
        if (VM.VM_Any === formatId) {
            var labels = []
            var values = []
            for (var i = 0; i < vod.formats; ++i) {
                var f = vod.format(i)
                labels.push(f.displayName)
                values.push(f.id)
            }

            var dialog = pageStack.push(
                Qt.resolvedUrl("SelectFormatDialog.qml"), {
                            "labels" : labels,
                            "values": values
                                        })
            dialog.accepted.connect(function() {
                _metaDataDownloadSucceededEx(token, vod, dialog.formatIndex)

            })
            return
        }
        var formatIndex = _findBestFormat(vod, formatId)
        _metaDataDownloadSucceededEx(token, vod, formatIndex)
    }

    function _findBestFormat(vod, formatId) {
        var formatIndex = -1
        if (VM.VM_Smallest === formatId) {
            var best = vod.format(0)
            formatIndex = 0
            for (var i = 1; i < vod.formats; ++i) {
                var f = vod.format(i)
                if (f.height < best.height) {
                    best = f;
                    formatIndex = i;
                }
            }
        } else if (VM.VM_Largest === formatId) {
            var best = vod.format(0)
            formatIndex = 0
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
                if (f.format === formatId) {
                    formatIndex = i
                    break
                }
            }

            if (formatIndex === -1) {
                var target = targetWidth(formatId)
                var bestdelta = Math.abs(vod.format(0).height - target)
                formatIndex = 0
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

        return formatIndex
    }

    function _metaDataDownloadSucceededEx(token, vod, formatIndex) {
        console.debug("formatIndex=" + formatIndex)
        var path = settingDefaultDirectory.value
        if (!path) {
            path = StandardPaths.download
        }

        var format = vod.format(formatIndex)
        var fileName = settingDefaultFileName.value
        fileName = fileName.replace("{title}", vod.description.title)
        fileName = fileName.replace("{id}", vod.description.id)
        fileName = fileName.replace("{formatid}", format.id)
        path = path + "/" + fileName + "." + format.fileExtension
        path = vodDownloadModel.sanatizePath(path)

        console.debug("format=" + format)
        console.debug("path=" + path)
        vodDownloadModel.startDownloadVod(token, vod, formatIndex, path)
    }

    function downloadFailed(url, error, filePath) {
        console.debug("url=" + url + ", error=" + error, ", path=" + filePath)
        switch (error) {
        case VM.VM_ErrorNone:
        case VM.VM_ErrorCanceled:
            return;
        case VM.VM_ErrorNoYoutubeDl:

            errorNotification.body = errorNotification.previewBody =
                    //% "youtube-dl not working"
                    qsTrId("error-youtube-dl-not-working")
            break
        case VM.VM_ErrorCrashed:
            errorNotification.body = errorNotification.previewBody =
                    //% "youtube-dl process crashed"
                    qsTrId("error-youtube-dl-process-crashed")
            break
        case VM.VM_ErrorTimedOut:
            errorNotification.body = errorNotification.previewBody =
                    //% "Download timed out"
                    qsTrId("error-download-timed-out")
            break
        case VM.VM_ErrorInvalidResponse:
            errorNotification.body = errorNotification.previewBody =
                    //% "Host delivered an invalid response to VOD format request"
                    qsTrId("error-invalid-response")
            break
        case VM.VM_ErrorUnsupportedUrl:
            errorNotification.body = errorNotification.previewBody =
                    //% "youtube-dl doesn't know how to handle that URL"
                    qsTrId("error-unsupported-url")
            break
        case VM.VM_ErrorNoVideo:
            errorNotification.body = errorNotification.previewBody =
                    //% "No video content for URL"
                    qsTrId("error-no-video")
            break
        case VM.VM_ErrorFormatNotAvailable:
            errorNotification.body = errorNotification.previewBody =
                    //% "The format you selected is not available. Try again or choose another format."
                    qsTrId("error-format-not-available")
            break
        case VM.VM_ErrorNetworkDown:
            errorNotification.body = errorNotification.previewBody =
                    //% "Network down."
                    qsTrId("error-network-down")
            break
        case VM.VM_ErrorInvalidUrl:
            errorNotification.body = errorNotification.previewBody =
                    //% "Invalid URL."
                    qsTrId("error-invalid-url")
            break
        case VM.VM_ErrorNoSpaceLeftOnDevice:
            errorNotification.body = errorNotification.previewBody =
                    //% "No space left on device."
                    qsTrId("error-no-space")
            break
        case VM.VM_ErrorAlreadyDownloading:
            errorNotification.body = errorNotification.previewBody =
                    //% "Already downloading %1"
                    qsTrId("error-already-downloading").arg(filePath)
            break
        default:
            errorNotification.body = errorNotification.previewBody =
                    //% "Yikes! An unknown error has occured :/"
                    qsTrId("error-unknown")
            break
        }

        errorNotification.publish()
    }

    function downloadSucceeded(download) {
        console.debug("download=" + download)
        successNotification.body = download.description.fullTitle
        successNotification.previewBody = download.description.fullTitle
        successNotification.remoteActions = [ {
                                                 "name": "default",
                                                 "displayName": qsTrId("play"), //% "Play"
                                                 "icon": "icon-cover-play",
                                                 "service": "org.duckdns.jgressmann.vodman.app",
                                                 "path": "/instance",
                                                 "iface": "org.duckdns.jgressmann.vodman.app",
                                                 "method": "play",
                                                 "arguments": [ download.filePath ]
                                             } ]
        successNotification.publish()
    }

    Component.onCompleted: {
        vodDownloadModel.metaDataDownloadSucceeded.connect(metaDataDownloadSucceeded)
        vodDownloadModel.downloadFailed.connect(downloadFailed)
        vodDownloadModel.downloadSucceeded.connect(downloadSucceeded)
    }

    DBusAdaptor {
        id: dbus
        bus: DBus.SessionBus
        service: 'org.duckdns.jgressmann.vodman.app'
        iface: 'org.duckdns.jgressmann.vodman.app'
        path: '/instance'

        function play(filePath) {
            console.debug("play path=" + filePath)
            Qt.openUrlExternally("file://" + filePath)
        }
    }

    Notification {
         id: errorNotification
         category: "x-nemo.transfer.error"
         //% "Download failed"
         summary: qsTrId("nofification-download-failed-summary")
         //% "Download failed"
         previewSummary: qsTrId("nofification-download-failed-summary")
    }

    Notification {
        id: successNotification
        category: "x-nemo.transfer.complete"
        appName: App.displayName
        appIcon: "/usr/share/icons/hicolor/86x86/apps/harbour-vodman.png"
        //% "Download finished"
        summary: qsTrId("nofification-download-finished-summary")
        //% "Download finished"
        previewSummary: qsTrId("nofification-download-finished-summary")
    }

    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                //% "About Vodman"
                text: qsTrId("menu-item-about-vodman").arg(App.displayName)
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                //% "Settings"
                text: qsTrId("menu-item-settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: "Download small video"
                visible: debugApp.value
                enabled: vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.startDownloadMetaData("https://www.youtube.com/watch?v=7t-l0q_v4D8")
            }

            MenuItem {
                text: "Download large video"
                visible: debugApp.value
                enabled: vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.startDownloadMetaData("https://www.twitch.tv/videos/161472611?t=07h49m09s")
            }

            MenuItem {
                text: "Download medium video"
                visible: debugApp.value
                enabled: vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.startDownloadMetaData("https://www.youtube.com/watch?v=KMAqSLWhH5w")
            }

            MenuItem {
                text: "Download reddit"
                visible: debugApp.value
                enabled: vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.startDownloadMetaData("https://www.reddit.com")
            }

            MenuItem {
                text: "Copy medium video url to clipboard"
                visible: debugApp.value
                onClicked: {
                    Clipboard.text = "https://www.youtube.com/watch?v=KMAqSLWhH5w"
                }
            }

            MenuItem {
                text: "Copy small video url to clipboard"
                visible: debugApp.value
//                enabled: vodDownloadModel.canStartDownload
                onClicked: {
                    Clipboard.text = "https://www.youtube.com/watch?v=7t-l0q_v4D8"
                }
            }

            MenuItem {
                text: "Copy reddit to clipboard"
                visible: debugApp.value
                onClicked: {
                    Clipboard.text = "https://www.reddit.com"
                }
            }

            MenuItem {
                //% "Clear clipboard"
                text: qsTrId("menu-item-clear-clipboard")
                visible: debugApp.value
                onClicked: {
                    Clipboard.text = ""
                }
            }

            MenuItem {
                //% "Cancel all downloads"
                text: qsTrId("menu-item-cancel-all-downloads")
                visible: listView.count > 0
                onClicked: {
                      remorse.execute(
                          //% "Stopping all downloads"
                          qsTrId("remorse-cancel-all-downloads"),
                          function() { vodDownloadModel.cancelDownloads(false) })
                }
            }

            MenuItem {
                //% "Cancel all downloads and delete files"
                text: qsTrId("menu-item-cancel-all-downloads-delete")
                visible: listView.count > 0
                onClicked: {
                      remorse.execute(
                          //% "Purging all downloads"
                          qsTrId("remorse-cancel-all-downloads-delete"),
                          function() { vodDownloadModel.cancelDownloads(true) })
                }
            }

            MenuItem {
                //% "Cancel"
                text: qsTrId("cancel")
                visible: !vodDownloadModel.canStartDownload
                onClicked: vodDownloadModel.cancelDownloadMetaData()
            }

            MenuItem {
                //% "Download from clipboard"
                text: qsTrId("menu-item-download-from-clipboard")
                enabled: canStartDownloadOfClipboardUrl
                onClicked: vodDownloadModel.startDownloadMetaData(Clipboard.text)
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
                //% "Downloads"
                title: qsTrId("download-page-header")

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
                width: ListView.view.width
                ListView.onRemove: animateRemoval(listItem)

                function cancelDownload(deleteFile) {
                    remorseAction(
                        //% "Stopping %1"
                        qsTrId("download-item-remorse-cancel").arg(download.data.description.fullTitle),
                        function() {
                            if (typeof(index) === "number") { // list item could have been removed
                                vodDownloadModel.cancelDownload(index, deleteFile)
                            }
                        })
                }

                ProgressOverlay {
                    anchors.fill: parent
                    progress: download.data.progress

                    Item {
                        height: parent.height
                        width: parent.width - 2 * x
                        x: Theme.paddingMedium

                        Image {
                            id: thumbnail
                            source: download.data.description.thumbnailUrl
                            width: parent.height
                            height: parent.height
                            sourceSize.width: width
                            sourceSize.height: height
                            fillMode: Image.PreserveAspectFit
                            cache: false
                            // prevents the image from loading on device
                            //asynchronous: true
                            visible: status === Image.Ready
                        }

                        Item {
                            visible: !thumbnail.visible
                            width: parent.height
                            height: parent.height

                            BusyIndicator {
                                size: BusyIndicatorSize.Medium
                                anchors.centerIn: parent
                                running: parent.visible
                            }
                        }

                        Item { // spacer
                            id: spacer
                            x: parent.height
                            width: Theme.paddingMedium
                            height: parent.height
                        }



    // aparently Thumbnail only works for local files
    //                        Thumbnail {
    //                            id: thumbnail
    //                            source: download.data.description.thumbnailUrl
    //                            width: parent.height
    //                            height: parent.height
    //                            sourceSize.width: width
    //                            sourceSize.height: height
    //                            fillMode: Image.PreserveAspectFit
    //                            priority: {
    //                                    if (page.status === PageStatus.Activating ||
    //                                        page.status === PageStatus.Active) {
    //                                            return Thumbnail.HighPriority
    //                                    }

    //                                    return Thumbnail.LowPriority
    //                            }


    ////                            visible: status === Thumbnail.Ready

    //                            onStatusChanged: {
    //                                console.debug("thumbnail status=" + status)
    //                            }
    //                        }



                        Column {
                            anchors.left: spacer.right
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter

                            Label {
                                id: title
                                width: parent.width
                                text: download.data.description.fullTitle
                                font.pixelSize: Theme.fontSizeSmall
                                truncationMode: TruncationMode.Fade
                            }

                            Label {
                                function makeSizeString(size) {
                                    var oneGb = 1000000000
                                    var oneMb = 1000000

                                    if (size >= 10*oneGb) { // 10GB
                                        return (size/oneGb).toFixed(1) + " GB"
                                    }

                                    if (size >= oneGb) { // 1GB
                                        return (size/oneGb).toFixed(2) + " GB"
                                    }

                                    return (size/oneMb).toFixed(0) + " MB"
                                }

                                width: parent.width
                                text: {
                                    var str = ""
                                    str += download.data.format.width
                                    str += "x"
                                    str += download.data.format.height
                                    if (download.data.fileSize > 0) {
                                        str += ", "
                                        str += makeSizeString(download.data.fileSize)
                                    }

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
                            //% "Cancel and delete file"
                            text: qsTrId("download-item-cancel-and-delete")
                            onClicked: cancelDownload(true)
                        }

                        MenuItem {
                            //% "Cancel"
                            text: qsTrId("cancel")
                            onClicked: cancelDownload(false)
                        }

                        MenuItem {
                            //% "Play"
                            text: qsTrId("play")
                            onClicked: Qt.openUrlExternally("file://" + download.data.filePath)
                        }

                        MenuItem {
                            //% "Open webpage"
                            text: qsTrId("download-item-open-webpage")
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
                    if (vodDownloadModel.downloadsPending) {
                        //% "Download will start momentarily"
                        return qsTrId("download-placeholder-text-waiting-for-download-to-start")
                    }

                    if (vodDownloadModel.canStartDownload) {
                        //% "No downloads at present"
                        return qsTrId("download-placeholder-text-no-downloads")
                    }

                    //% "Downloading VOD metadata"
                    return qsTrId("download-placeholder-text-metadata-download")
                }

                hintText: {
                    if (vodDownloadModel.downloadsPending) {
                        return ""
                    }

                    if (!vodDownloadModel.downloadsPending && vodDownloadModel.canStartDownload) {
                        if (clipBoardHasUrl) {
                            //% "Pull down to start download using the URL in the clipboard"
                            return qsTrId("download-placeholder-hint-pull-down-to-start-download-from-clipboard")
                        }

                        //% "Copy a URL to the clipboard then pull down to start the download"
                        return qsTrId("download-placeholder-hint-copy-url-to-clipboard")
                    }

                    //% "Pull down to cancel"
                    return qsTrId("download-placeholder-hint-pull-down-to-cancel")
                }
            }
        }
    }
}

