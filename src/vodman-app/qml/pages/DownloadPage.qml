/* The MIT License (MIT)
 *
 * Copyright (c) 2018-2024 Jean Gressmann <jean@0x42.de>
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
import Vodman 2.1

import ".."


Page {
    id: page

    readonly property bool isDownloadPage: true
    // page could be underneath settings page
    readonly property bool operational: YTDLDownloader.downloadStatus === YTDLDownloader.StatusReady
    readonly property bool canStartDownload: operational
    readonly property bool clipBoardHasUrl: Clipboard.hasText && vodDownloadModel.isUrl(Clipboard.text)
    readonly property bool canStartDownloadOfClipboardUrl: canStartDownload && clipBoardHasUrl

    function _download(url) {
        vodDownloadModel.startDownloadMetaData(vodDownloadModel.newToken(), url)
    }

    function _selectVideoFormat(vod, more) {
        if (vod.videoFormats > 1) {
            var labels = []
            var values = []
            var formats = {}

            for (var i = 0; i < vod.videoFormats; ++i) {
                var f = vod.videoFormat(i)
                var res = f.width + "x" + f.height;
                formats[res] = i;
            }

            for (res in formats) {
                var format_index = formats[res];
                labels.push(res);
                values.push(format_index);
            }

            var dialog = pageStack.push(
                Qt.resolvedUrl("SelectFormatDialog.qml"), {
                            //% "Select video format"
                            "title": qsTrId("select-video-format-dialog-title"),
                            "labels" : labels,
                            "values": values,
                        })
            dialog.accepted.connect(function() {
                more(values[dialog.formatIndex])
            })
        } else {
            more(0)
        }
    }

    function _selectAudioFormat(vod, more) {
        if (vod.audioFormats > 1) {
            var labels = []
            var values = []

            for (var i = 0; i < vod.audioFormats; ++i) {
                var f = vod.audioFormat(i)
                labels.push(f.displayName + " / " + f.abr.toFixed(0) + " [abr] " + f.extension)
                values.push(f.id)
            }

            var dialog = pageStack.push(
                Qt.resolvedUrl("SelectFormatDialog.qml"), {
                            //% "Select audio format"
                            "title": qsTrId("select-audio-format-dialog-title"),
                            "labels" : labels,
                            "values": values
                        })
            dialog.accepted.connect(function() {
                more(dialog.formatIndex)
            })
        } else {
            more(0)
        }
    }

    function _selectAvFormat(vod, more) {
        if (vod.avFormats > 1) {
            var labels = []
            var values = []
            var formats = {}

            for (var i = 0; i < vod.avFormats; ++i) {
                var f = vod.avFormat(i)
                var key = f.displayName  + " [" + f.extension + "]";
                formats[key] = i;
            }

            for (var key in formats) {
                var format_index = formats[key];
                labels.push(key);
                values.push(format_index);
            }

            var dialog = pageStack.push(
                Qt.resolvedUrl("SelectFormatDialog.qml"), {
                            //% "Select a format"
                            "title": qsTrId("select-av-format-dialog-title"),
                            "labels" : labels,
                            "values": values
                        })
            dialog.accepted.connect(function() {
                more(values[dialog.formatIndex])
            })
        } else {
            more(0)
        }
    }

    function metaDataDownloadSucceeded(token, playlist) {
//        console.debug("token=" + token + ", playlist=" + playlist)
        console.debug("#vods=" + playlist.vods)

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
            var vod = playlist.vod(0)

            if (vod.videoFormats > 0) {
                _selectVideoFormat(vod, function (formatIndex) {
                    var f = vod.videoFormat(formatIndex)
                    _metaDataDownloadSucceededEx(token, playlist, "height:" + f.height + ",width:" + f.width, f.width + "x" + f.height)
                })
                return
            }

            if (vod.avFormats > 0) {
                _selectAvFormat(vod, function (formatIndex) {
                    var f = vod.avFormat(formatIndex)
                    _metaDataDownloadSucceededEx(token, playlist, "height:" + f.height + ",width:" + f.width + ",ext:" + f.extension, f.width + "x" + f.height)
                })
                return
            }
        }

        var res = _findBestFormat(playlist, formatId)
        var search = res[0]
        var displayFormat = res[1]
        _metaDataDownloadSucceededEx(token, playlist, search, displayFormat)
    }

    function _findBestFormat(playlist, formatId) {
        if (VM.VM_Smallest === formatId) {
            //% "worst"
            return ["+res", qsTrId("format-worst")]
        }

        if (VM.VM_Largest === formatId) {
            //% "best"
            return ["res", qsTrId("format-best")]
        }

        var height
        var width
        switch (formatId) {
        case VM.VM_240p:
            height = 240
            width = 426
            break
        case VM.VM_360p:
            height = 360
            width = 640
            break
        case VM.VM_720p:
            height = 720
            width = 1280
            break
        default:
            height = 1080
            width = 1920
            break
        }

        return ["height:" + height, "~" + width + "x" + height]
    }

    function _metaDataDownloadSucceededEx(token, playlist, search, displayFormat) {
        var path = settingDefaultDirectory.value
        if (!path) {
            path = StandardPaths.download
        }

        var fileName = settingDefaultFileName.value
        fileName = fileName.replace("{title}", "%(title)s")
        fileName = fileName.replace("{id}", "%(id)s")
//        fileName = fileName.replace("{formatid}", format.id)
//        if (playlist.vods > 1) { // youtube-dl meta var
//            fileName += "_%(playlist_index)s"
//        }
        path = path + "/" + fileName + ".%(ext)s"

        console.debug("search=" + search)
        console.debug("path=" + path)
        vodDownloadModel.startDownloadPlaylist(token, playlist, search, path, displayFormat)
    }

    function downloadFailed(url, error, filePath) {
        console.debug("url=" + url + ", error=" + error, ", path=" + filePath)
        switch (error) {
        case VM.VM_ErrorNone:
        case VM.VM_ErrorCanceled:
            return;
        case VM.VM_ErrorNoYoutubeDl:
            errorNotification.body = errorNotification.previewBody =
                    //% "%1 not working"
                    qsTrId("error-youtube-dl-not-working").arg(YTDLDownloader.ytdlName)
            break
        case VM.VM_ErrorCrashed:
            errorNotification.body = errorNotification.previewBody =
                    //% "%1 process crashed"
                    qsTrId("error-youtube-dl-process-crashed").arg(YTDLDownloader.ytdlName)
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
                    //% "%1 doesn't know how to handle that URL"
                    qsTrId("error-unsupported-url").arg(YTDLDownloader.ytdlName)
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
            //% "Invalid URL"
            errorNotification.previewBody = qsTrId("error-invalid-url-preview-body")
            //% "Invalid URL '%1'"
            errorNotification.body = qsTrId("error-invalid-url-body").arg(url)
            break
        case VM.VM_ErrorNoSpaceLeftOnDevice:
            errorNotification.body = errorNotification.previewBody =
                    //% "No space left on device."
                    qsTrId("error-no-space")
            break
        case VM.VM_ErrorAlreadyDownloading:
            //% "Already downloading %1"
            transientNotification.previewSummary = transientNotification.summary = qsTrId("error-already-downloading").arg(url)
            transientNotification.previewBody = errorNotification.previewBody = filePath
            transientNotification.publish()
            return
        case VM.VM_ErrorContentGone:
            //% "Video gone"
            errorNotification.previewBody = qsTrId("error-content-gone-preview-body")
            //% "Video '%1' is no longer available."
            errorNotification.body = qsTrId("error-content-gone-body").arg(url)
            break
        case VM.VM_ErrorSignatureExtractionFailed:
            //% "Failed to locate video signature. Update %1 and try again."
            errorNotification.previewBody = errorNotification.body = qsTrId("error-signature-extraction-failed-body").arg(YTDLDownloader.ytdlName)
            break
        case VM.VM_ErrorTemporaryFailureNameResolution:
            //% "Temporary name resolution failure. Try again later."
            errorNotification.previewBody = errorNotification.body = qsTrId("error-temporary-name-resolution-failure-body")
            break
        case VM.VM_ErrorAccess:
            //% "Access denied"
            errorNotification.previewBody = errorNotification.body = qsTrId("error-access")
            break
        case VM.VM_ErrorTooManyRequests:
            //% "Too many requests. Try again later or update %1."
            errorNotification.previewBody = errorNotification.body = qsTrId("error-too-many-requests").arg(YTDLDownloader.ytdlName)
            break
        case VM.VM_ErrorSslCertVerifyFailed:
            //% "SSL certificate verfication failed."
            errorNotification.previewBody = errorNotification.body = qsTrId("error-ssl-cert-verify").arg(YTDLDownloader.ytdlName)
            break
        case VM.VM_ErrorDifferentFormats:
            //% "Different streams/codecs detected. Files may be incomplete and/or not play."
            errorNotification.previewBody = errorNotification.body = qsTrId("error-different-streams-codecs").arg(YTDLDownloader.ytdlName)
            break
        default:
            errorNotification.body = errorNotification.previewBody =
                    //% "Yikes! An unknown error has occured :/"
                    qsTrId("error-unknown")
            break
        }

        errorNotification.remoteActions = [ {
                                               "name": "default",
                                               //% "Retry"
                                               "displayName": qsTrId("retry"),
                                               "icon": "icon-cover-refresh",
                                               "service": "org.duckdns.jgressmann.vodman.app",
                                               "path": "/instance",
                                               "iface": "org.duckdns.jgressmann.vodman.app",
                                               "method": "download",
                                               "arguments": [ url ]
                                           } ]

        errorNotification.publish()
    }

    function downloadSucceeded(download) {
        console.debug("download=" + download)
        successNotification.body = download.playlist.title
        successNotification.previewBody = download.playlist.title
        successNotification.remoteActions = [ {
                                                 "name": "default",
                                                 //% "Play"
                                                 "displayName": qsTrId("play"),
                                                 "icon": "icon-cover-play",
                                                 "service": "org.duckdns.jgressmann.vodman.app",
                                                 "path": "/instance",
                                                 "iface": "org.duckdns.jgressmann.vodman.app",
                                                 "method": "play",
                                                 "arguments": [ download.file(0).filePath ]
                                             } ]
        successNotification.publish()
    }

    Component.onCompleted: {
        vodDownloadModel.metaDataDownloadSucceeded.connect(metaDataDownloadSucceeded)
        vodDownloadModel.downloadFailed.connect(downloadFailed)
        vodDownloadModel.downloadSucceeded.connect(downloadSucceeded)
    }

    Component.onDestruction: {
        vodDownloadModel.metaDataDownloadSucceeded.disconnect(metaDataDownloadSucceeded)
        vodDownloadModel.downloadFailed.disconnect(downloadFailed)
        vodDownloadModel.downloadSucceeded.disconnect(downloadSucceeded)
    }

    DBusAdaptor {
        id: dbus
        bus: DBus.SessionBus
        service: 'org.duckdns.jgressmann.vodman.app'
        iface: 'org.duckdns.jgressmann.vodman.app'
        path: '/instance'
        xml:
"<interface name=\"org.duckdns.jgressmann.vodman.app\">\n" +
"   <property name=\"canStartDownload\" type=\"boolean\" access=\"readonly\"/>\n" +
"   <method name=\"download\">\n" +
"       <arg name=\"url\" type=\"s\" direction=\"in\"/>\n" +
"   </method>\n" +
"   <method name=\"downloadEx\">\n" +
"       <arg name=\"url\" type=\"s\" direction=\"in\"/>\n" +
"       <arg name=\"notify\" type=\"b\" direction=\"in\"/>\n" +
"       <arg name=\"success\" type=\"b\" direction=\"out\"/>\n" +
"   </method>\n" +
"</interface>\n"

        readonly property bool canStartDownload: page.canStartDownload
        function play(filePath) {
            console.debug("play path=" + filePath)
            Qt.openUrlExternally("file://" + filePath)
        }

        function download(url) {
            downloadEx(url, true)
        }

        function downloadEx(url, notify) {
            console.debug("download url=" + url + ", notify=" + notify)
            if (canStartDownload) {
                _download(url)
                if (notify) {
                    //% "Started download of '%1'"
                    startedNotification.summary = qsTrId("nofification-download-started-summary").arg(url)
                    startedNotification.publish()
                }

                return true
            }

            if (notify) {
                errorNotification.body = errorNotification.previewBody =
                        //% "%1 is busy. Try again later."
                        qsTrId("notification-busy").arg(App.displayName)
                errorNotification.publish()
            }

            return false
        }

        function updateYtdl() {
            if (!canStartDownload || vodDownloadModel.busy) {
                errorNotification.body = errorNotification.previewBody =
                        //% "%1 is busy. Try again later."
                        qsTrId("notification-busy").arg(App.displayName)
                errorNotification.publish()
            } else {
                YTDLDownloader.download()
            }
        }
    }

    Notification {
         id: errorNotification
         appName: App.displayName
         appIcon: "/usr/share/icons/hicolor/86x86/apps/harbour-vodman.png"
         icon: appIcon
         //% "Download failed"
         summary: qsTrId("nofification-download-failed-summary")
         previewSummary: summary
    }

    Notification {
         id: transientNotification
         appName: App.displayName
         appIcon: "/usr/share/icons/hicolor/86x86/apps/harbour-vodman.png"
         icon: appIcon
         isTransient: true
    }

    Notification {
        id: successNotification
        category: "x-nemo.transfer.complete"
        appName: App.displayName
        appIcon: "/usr/share/icons/hicolor/86x86/apps/harbour-vodman.png"
        icon: appIcon
        //% "Download finished"
        summary: qsTrId("nofification-download-finished-summary")
        previewSummary: summary
    }

    Notification {
        id: startedNotification
        appName: App.displayName
        appIcon: "/usr/share/icons/hicolor/86x86/apps/harbour-vodman.png"
        icon: appIcon
        //% "Download started"
        previewSummary: qsTrId("nofification-download-started-preview-summary")
        isTransient: true
    }

    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                //% "About %1"
                text: qsTrId("menu-item-about-vodman").arg(App.displayName)
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

            MenuItem {
                //% "Settings"
                text: qsTrId("menu-item-settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: "Download m3u8"
                visible: debugApp.value
                enabled: page.canStartDownload
                onClicked: _download(
//                               "https://multiplatform-f.akamaihd.net/i/multi/will/bunny/big_buck_bunny_,640x360_400,640x360_700,640x360_1000,950x540_1500,.f4v.csmil/master.m3u8"
                               "https://demo.unified-streaming.com/k8s/features/stable/video/tears-of-steel/tears-of-steel.ism/.m3u8"
                               )
            }

            MenuItem {
                text: "Download large video"
                visible: debugApp.value
                enabled: page.canStartDownload
                onClicked: _download("https://www.twitch.tv/videos/161472611?t=07h49m09s")
            }

            MenuItem {
                text: "Download medium video"
                visible: debugApp.value
                enabled: page.canStartDownload
                onClicked: _download("https://www.youtube.com/watch?v=KMAqSLWhH5w")
            }

            MenuItem {
                text: "Download afreecatv playlist video"
                visible: debugApp.value
                enabled: page.canStartDownload
                onClicked: _download("http://vod.afreecatv.com/PLAYER/STATION/42458592")
                //onClicked: _download("http://vod.afreecatv.com/PLAYER/STATION/42098913")
            }

            MenuItem {
                text: "Download youtube playlist"
                visible: debugApp.value
                enabled: page.canStartDownload
                onClicked: _download("https://www.youtube.com/playlist?list=PLB4brr7vf-P7pT4mcWuAZJ79o-38RZ3Bx")
            }

            MenuItem {
                text: "Download reddit"
                visible: debugApp.value
                enabled: page.canStartDownload
                onClicked: _download("https://www.reddit.com")
            }

            MenuItem {
                text: "Download mp4"
                visible: debugApp.value
                enabled: page.canStartDownload
                onClicked: _download("http://techslides.com/demos/samples/sample.mp4")
            }

            MenuItem {
                text: "HTTP 404"
                visible: debugApp.value
                enabled: page.canStartDownload
                onClicked: _download("https://rodlzdf-a.akamaihd.net/none/zdf/19/03/190329_sendung_hsh/3/190329_sendung_hsh_3328k_p36v14.mp4")
            }

            MenuItem {
                text: "Download png"
                visible: debugApp.value
                enabled: page.canStartDownload
                onClicked: _download("https://openrepos.net/sites/default/files/openrepos_beta.png")
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
                text: "Copy m3u8 to clipboard"
                visible: debugApp.value
                onClicked: {
                    Clipboard.text = "http://videofile-hls-ko-record-cf.afreecatv.com/video/_definst_/vod/20190317/085/806BF397_212355085_1.smil/chunklist_b1000000_t64aGQ=.m3u8"
                }
            }

            MenuItem {
                text: "Delete youtube-dl"
                visible: debugApp.value &&
                         !vodDownloadModel.busy &&
                         YTDLDownloader.downloadStatus == YTDLDownloader.StatusReady
                onClicked: YTDLDownloader.remove()
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
                //% "Download from clipboard"
                text: qsTrId("menu-item-download-from-clipboard")
                enabled: page.canStartDownloadOfClipboardUrl
                onClicked: _download(Clipboard.text)
            }
        }

        contentWidth: parent.width


        VerticalScrollDecorator {}

        SilicaListView {
            id: listView
            visible: YTDLDownloader.downloadStatus === YTDLDownloader.StatusDownloading || YTDLDownloader.downloadStatus === YTDLDownloader.StatusReady
            anchors.fill: parent
            model: vodDownloadModel
            header: PageHeader {
                //% "Downloads"
                title: qsTrId("download-page-header")

                BusyIndicator {
                    running: YTDLDownloader.downloadStatus === YTDLDownloader.StatusDownloading || vodDownloadModel.metaDataDownloadsPending
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
                        qsTrId("download-item-remorse-cancel").arg(download.data.playlist.title),
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
                            source: {
                                var thumbnailUrl = download.data.playlist.vod(download.data.currentFileIndex).thumbnailUrl
                                if (thumbnailUrl) {
                                    return thumbnailUrl
                                }

                                return "image://theme/icon-l-image"
                            }
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
                                text: download.data.playlist.vod(download.data.currentFileIndex).title
                                font.pixelSize: Theme.fontSizeSmall
                                truncationMode: TruncationMode.Fade
                            }

                            Label {
                                function makeSizeString(size) {
                                    var oneGb = 1000000000
                                    var oneMb = 1000000

                                    if (size >= 10*oneGb) { // 10GB
                                        //% "%1 GB"
                                        return qsTrId("unit-gb").arg((size/oneGb).toFixed(1))
                                    }

                                    if (size >= oneGb) { // 1GB
                                        //% "%1 GB"
                                        return qsTrId("unit-gb").arg((size/oneGb).toFixed(2))
                                    }

                                    //% "%1 MB"
                                    return qsTrId("unit-mb").arg((size/oneMb).toFixed(0))
                                }

                                width: parent.width
                                text: {
                                    var str = ""
                                    if (download.data.userData) {
                                        str = "" + download.data.userData
                                    }

                                    if (download.data.playlist.vods > 1) {
                                        if (str.length > 0) {
                                            str += ", "
                                        }

                                        //% "file %1"
                                        str += qsTrId("download-item-vod-file").arg((download.data.currentFileIndex + 1) + "/" + download.data.playlist.vods)
                                    }

                                    if (download.data.fileSize) {
                                        if (str.length > 0) {
                                            str += ", "
                                        }
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
                                text: download.data.file(download.data.currentFileIndex).filePath
                                font.pixelSize: Theme.fontSizeTiny
                                color: Theme.secondaryColor
                                truncationMode: TruncationMode.Fade
                            }

                            LinkedLabel {
                                width: parent.width
                                plainText: download.data.playlist.webPageUrl
                                font.pixelSize: Theme.fontSizeTiny
                                shortenUrl: true
                            }
                        }
                    }
                }

                Component {
                    id: contextMenu
                    ContextMenu {
                        MenuItem {
                            //% "Cancel and delete file"
                            text: qsTrId("download-item-cancel-and-delete", download.data.playlist.vods)
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
                            visible: !!download.data.file(0).filePath
                            onClicked: Qt.openUrlExternally("file://" + download.data.file(0).filePath)
                        }

                        MenuItem {
                            //% "Open webpage"
                            text: qsTrId("download-item-open-webpage")
                            onClicked: {
                                console.debug("opening: " + download.data.playlist.webPageUrl)
                                Qt.openUrlExternally(download.data.playlist.webPageUrl)
                            }
                        }

                        MenuItem {
                            visible: !!download.data.file(0).filePath
                            //% "Copy file path to clipboard"
                            text: qsTrId("download-item-copy-file-path-to-clipboard")
                            onClicked: Clipboard.text = download.data.file(0).filePath
                        }
                    }
                }
            }

            ViewPlaceholder {
                enabled: listView.count === 0
                text: {
                    if (vodDownloadModel.metaDataDownloadsPending) {
                        //% "Downloading VOD metadata"
                        return qsTrId("download-placeholder-text-metadata-download")
                    }

                    if (vodDownloadModel.busy) {
                        //% "Download will start momentarily"
                        return qsTrId("download-placeholder-text-waiting-for-download-to-start")
                    }

                    //% "No downloads at present"
                    return qsTrId("download-placeholder-text-no-downloads")
                }

                hintText: {
                    if (vodDownloadModel.busy) {
                        return ""
                    }

                    if (clipBoardHasUrl) {
                        //% "Pull down to start download using the URL in the clipboard"
                        return qsTrId("download-placeholder-hint-pull-down-to-start-download-from-clipboard")
                    }

                    //% "Copy a URL to the clipboard then pull down to start the download"
                    return qsTrId("download-placeholder-hint-copy-url-to-clipboard")
                }
            }
        }
    }
}

