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
import org.duckdns.jgressmann 1.0
import Vodman 2.1
import "."


ApplicationWindow {
    id: window
    initialPage: YTDLDownloader.downloadStatus === YTDLDownloader.StatusReady
                 ? Qt.resolvedUrl("pages/DownloadPage.qml")
                 : Qt.resolvedUrl("pages/YTDLPage.qml")
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    allowedOrientations: defaultAllowedOrientations
    property bool _hasCheckedForYtdlUpdate: false


    DownloadModel {
        id: vodDownloadModel
        ytdl: YTDL
    }

    ConfigValue {
        id: settingBroadbandDefaultFormat
        defaultValue: VM.VM_Largest
        key: "/default/format/broadband"
    }

    ConfigValue {
        id: settingMobileDefaultFormat
        defaultValue: VM.VM_Smallest
        key: "/default/format/mobile"
    }

    ConfigValue {
        id: settingBearerMode
        defaultValue: Global.bearerModeAutoDetect
        key: "/bearer/mode"
    }

    ConfigValue {
        id: settingDefaultDirectory
        key: "/default/directory"
        defaultValue: StandardPaths.videos
    }

    ConfigValue {
        id: settingDefaultFileName
        key: "/default/fileName"
        defaultValue: "{title}"
    }

    ConfigValue {
        id: debugApp
        key: "/debug"
        defaultValue: false

        onValueChanged: {
            YTDL.ytdlVerbose = value
            _setMode()
        }
    }

    Component.onCompleted: {
        YTDL.cacheDirectory = StandardPaths.cache + "/youtube-dl/cache"
        YTDL.ytdlVerbose = debugApp.value
        YTDLDownloader.isUpdateAvailableChanged.connect(_ytdlUpdateAvailableChanged)
        _setMode()
        _setYtdlPath()
        _checkForYtdlUpate()
    }

    Component.onDestruction: {
        YTDLDownloader.isUpdateAvailableChanged.disconnect(_ytdlUpdateAvailableChanged)
    }

    Connections {
        target: YTDLDownloader
        onYtdlPathChanged: _setYtdlPath()

        onDownloadStatusChanged: {
            _switchMainPage()
            _checkForYtdlUpate()
        }
        onIsOnlineChanged: _checkForYtdlUpate()
    }

    Connections {
        target: pageStack
        onCurrentPageChanged: _switchMainPage()
        onBusyChanged: _switchMainPage()
    }

    Notification {
        id: updateNotification
        appName: App.displayName
        appIcon: "/usr/share/icons/hicolor/86x86/apps/harbour-vodman.png"
        icon: appIcon
        //% "youtube-dl update available"
        summary: qsTrId("nofification-download-ytdl-update-available-summary")
        previewSummary: summary
        //% "youtube-dl version %1 available"
        body: qsTrId("nofification-ytdl-update-available-body").arg(YTDLDownloader.updateVersion)
        previewBody: body
        remoteActions: [ {
             "name": "default",
             //% "Update youtube-dl"
             "displayName": qsTrId("nofification-ytdl-update-available-action"),
             "service": "org.duckdns.jgressmann.vodman.app",
             "path": "/instance",
             "iface": "org.duckdns.jgressmann.vodman.app",
             "method": "updateYtdl",
         } ]
    }



    function _setYtdlPath() {
        if (YTDLDownloader.downloadStatus === YTDLDownloader.StatusReady) {
            YTDL.ytdlPath = YTDLDownloader.ytdlPath
        } else {
            YTDL.ytdlPath = ""
        }
    }

    function _setMode() {
        YTDLDownloader.mode = debugApp.value ? YTDLDownloader.ModeTest : YTDLDownloader.ModeRelease
    }

    function _switchMainPage() {
        if (pageStack.currentPage && !pageStack.busy) {
            if (YTDLDownloader.downloadStatus === YTDLDownloader.StatusReady) {
                if (pageStack.currentPage.isYTDLPage) {
                    pageStack.replace(Qt.resolvedUrl("pages/DownloadPage.qml"))
                }
            } else {
                if (pageStack.currentPage.isDownloadPage) {
                    pageStack.replace(Qt.resolvedUrl("pages/YTDLPage.qml"))
                }
            }
        }
    }

    function _ytdlUpdateAvailableChanged() {
        if (YTDLDownloader.isUpdateAvailable) {
            updateNotification.publish()
        }
    }

    function _checkForYtdlUpate() {
        if (!_hasCheckedForYtdlUpdate &&
            YTDLDownloader.isOnline &&
            YTDLDownloader.downloadStatus === YTDLDownloader.StatusReady &&
            YTDLDownloader.updateStatus === YTDLDownloader.StatusUnavailable) {
            _hasCheckedForYtdlUpdate = true
            YTDLDownloader.checkForUpdate()
        }
    }
}

