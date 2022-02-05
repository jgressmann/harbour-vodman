/* The MIT License (MIT)
 *
 * Copyright (c) 2019-2022 Jean Gressmann <jean@0x42.de>
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
import Vodman 2.1


Page {
    readonly property bool isYTDLPage: true
    property string _downloadName


    SilicaFlickable {
        anchors.fill: parent
        contentWidth: parent.width
        contentHeight: parent.height

        Column {
            x: Theme.horizontalPageMargin
            width: parent.width - 2*x
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2*Theme.paddingMedium

            Column {
                width: parent.width

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: {
                        switch (YTDLDownloader.downloadStatus) {
                        case YTDLDownloader.StatusUnavailable:
                            //% "You seem to be missing a working %1"
                            return qsTrId("ytdl-unvailable").arg(YTDLDownloader.ytdlDefaultName)
                        case YTDLDownloader.StatusError:
                            //% "There was an error downloading %1"
                            return qsTrId("ytdl-error").arg(_downloadName)
                        case YTDLDownloader.StatusDownloading:
                            //% "%1 is being downloaded"
                            return qsTrId("ytdl-downloading").arg(_downloadName)
                        case YTDLDownloader.StatusReady:
                            return ""
                        default:
                            return "YTDLPage FIX ME"
                        }
                    }

                    font {
                        pixelSize: Theme.fontSizeExtraLarge
                        family: Theme.fontFamilyHeading
                    }
                    color: Theme.secondaryHighlightColor
                }

                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: {
                        switch (YTDLDownloader.downloadStatus) {
                        case YTDLDownloader.StatusUnavailable:
                            if (YTDLDownloader.isOnline) {
                                //% "Click the button below to start the download"
                                return qsTrId("ytdl-unvailable-hint")
                            }

                            //% "Go online, then click the button below to start the download"
                            return qsTrId("ytdl-unvailable-hint-offline")
                        case YTDLDownloader.StatusError:
                            //% "Click the button below to try again"
                            return qsTrId("ytdl-error-hint")
                        case YTDLDownloader.StatusDownloading:
                            //% "Please be patient"
                            return qsTrId("ytdl-downloading-hint")
                        case YTDLDownloader.StatusReady:
                            return ""
                        default:
                            return "YTDLPage FIX ME"
                        }
                    }

                    font {
                        pixelSize: Theme.fontSizeLarge
                        family: Theme.fontFamilyHeading
                    }
                    color: Theme.highlightColor
                    opacity: 0.4
                }

            }

            Button {
                visible: !(YTDLDownloader.downloadStatus === YTDLDownloader.StatusDownloading || YTDLDownloader.downloadStatus === YTDLDownloader.StatusReady)
                enabled: YTDLDownloader.isOnline
                anchors.horizontalCenter: parent.horizontalCenter
                //% "Download"
                text: qsTrId("ytdl-download-button")
                onClicked: YTDLDownloader.download()
            }

            BusyIndicator {
                visible: running
                running: YTDLDownloader.downloadStatus === YTDLDownloader.StatusDownloading
                anchors.horizontalCenter: parent.horizontalCenter
                size: BusyIndicatorSize.Large
            }

            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                visible: YTDLDownloader.error !== YTDLDownloader.ErrorNone
                text: {
                    switch (YTDLDownloader.error) {
                    case YTDLDownloader.ErrorBusy:
                        //% "Try again later"
                        return qsTrId("ytdl-error-busy")
                    case YTDLDownloader.ErrorDownloadFailed:
                        //% "Download failed"
                        return qsTrId("ytdl-error-download-failed")
                    case YTDLDownloader.ErrorFileOrDirCreationFailed:
                        //% "Failed to create file/directory"
                        return qsTrId("ytdl-error-file-dir-creation")
                    case YTDLDownloader.ErrorMalformedConfigurationFile:
                        //% "Malformed configuration file. Try to download again."
                        return qsTrId("ytdl-error-config-malformed")
                    case YTDLDownloader.ErrorNoSpaceLeftOnDevice:
                        //% "No space left on device"
                        return qsTrId("ytdl-error-no-space")
                    case YTDLDownloader.ErrorPython:
                        //% "Python executable not found"
                        return qsTrId("ytdl-error-python")
                    case YTDLDownloader.ErrorUnknown:
                        //% "An unknown error has occurred"
                        return qsTrId("ytdl-error-unknown")
                    case YTDLDownloader.ErrorUnsupportedConfigurationFileFormat:
                        //% "Unsupported configuration file format. Try to update the application."
                        return qsTrId("ytdl-error-unsupported-config-file-format")
                    case YTDLDownloader.ErrorNone:
                        //% "All is well"
                        return qsTrId("ytdl-error-none")
                    case YTDLDownloader.ErrorOffline:
                        //% "Your device is offline"
                        return qsTrId("ytdl-error-offline")
                    default:
                        return "YTDLPage FIX ME"
                    }
                }

                font {
                    pixelSize: Theme.fontSizeMedium
                    family: Theme.fontFamilyHeading
                }
                color: Theme.secondaryHighlightColor
            }
        }
    }

    Component.onCompleted: {
        _downloadName = YTDLDownloader.isUpdateAvailable ? YTDLDownloader.updateName : YTDLDownloader.ytdlDefaultName
    }
}


