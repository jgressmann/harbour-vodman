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
import Nemo.Configuration 1.0
import org.duckdns.jgressmann 1.0
import "."




ApplicationWindow {
    id: window
    initialPage: Qt.resolvedUrl("pages/DownloadPage.qml")
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    allowedOrientations: defaultAllowedOrientations

    readonly property bool clipBoardHasUrl: Clipboard.hasText && !!Clipboard.text && vodDownloadModel.isUrl(Clipboard.text)
    readonly property bool canStartDownloadOfClipboardUrl: vodDownloadModel.canStartDownload && !_downloadingClipboardUrl && clipBoardHasUrl
    property bool _downloadingClipboardUrl: false

    VodDownloadModel {
        id: vodDownloadModel
    }

    ConfigurationGroup {
        id: settings

        ConfigurationValue {
            id: settingBroadbandDefaultFormat
            defaultValue: VM.VM_Largest
            key: "/default/format/broadband"
        }

        ConfigurationValue {
            id: settingMobileDefaultFormat
            defaultValue: VM.VM_Smallest
            key: "/default/format/mobile"
        }

        ConfigurationValue {
            id: settingBearerMode
            defaultValue: Global.bearerModeAutoDetect
            key: "/bearer/mode"
        }

        ConfigurationValue {
            id: settingDefaultDirectory
            key: "/default/directory"
            defaultValue: StandardPaths.videos
        }

        ConfigurationValue {
            id: settingDefaultFileName
            key: "/default/fileName"
            defaultValue: "{title}"
        }

        ConfigurationValue {
            id: debugApp
            key: "/debug"
            defaultValue: false
        }
    }

    Component.onCompleted: {
        vodDownloadModel.userDownloadsChanged.connect(_update)
        _update()
    }

    Connections {
        target: Clipboard
        onTextChanged: _update()
    }

    function _update() {
        if (clipBoardHasUrl) {
            _downloadingClipboardUrl = vodDownloadModel.isDownloading(Clipboard.text)
        } else {
            _downloadingClipboardUrl = false
        }

        console.debug("_downloadingClipboardUrl=" + _downloadingClipboardUrl)
    }

    onCanStartDownloadOfClipboardUrlChanged: {
        console.debug("can start download of URL in clipboard: " + canStartDownloadOfClipboardUrl)
    }

    onClipBoardHasUrlChanged: {
        console.debug("clipboard has URL: " + clipBoardHasUrl)
    }
}

