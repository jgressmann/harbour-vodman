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

#include "VMDownload.h"


#include <QDebug>
#include <QDebugStateSaver>

VMMetaDataDownloadData::VMMetaDataDownloadData()
{
    error = VMVodEnums::VM_ErrorNone;
}

VMMetaDataDownload::VMMetaDataDownload()
    : d(new VMMetaDataDownloadData())
{}

bool VMMetaDataDownload::isValid() const
{
    return  error() == VMVodEnums::VM_ErrorNone &&
            playlist().isValid();
}

bool VMPlaylistDownloadRequest::isValid() const
{
    return !filePath.isEmpty() &&
            !format.isEmpty() &&
            playlist.isValid();
}

VMFileDownloadData::VMFileDownloadData()
{
    fileSize = 0;
    progress = 0;
    playlistIndex = 0;
}

VMFileDownload::VMFileDownload()
    : d(new VMFileDownloadData())
{}


bool VMFileDownload::isValid() const
{
    return !filePath().isEmpty() &&
            progress() >= 0;
}

VMPlaylistDownloadData::VMPlaylistDownloadData()
{
    fileSize = 0;
    progress = 0;
    error = VMVodEnums::VM_ErrorNone;
    currentFileIndex = 0;
}

VMPlaylistDownload::VMPlaylistDownload()
    : d(new VMPlaylistDownloadData())
{}


bool VMPlaylistDownload::isValid() const
{
    return  error() == VMVodEnums::VM_ErrorNone &&
            playlist().isValid() &&
            files() > 0 &&
            currentFileIndex() >= 0 &&
            currentFileIndex() < files();
}

QVariant VMPlaylistDownload::file(int index) const
{
    if (index >= 0 && index < d->files.size()) {
        return QVariant::fromValue(d->files[index]);
    }

    return QVariant();
}

QDebug operator<<(QDebug debug, const VMMetaDataDownload& value) {
    const VMMetaDataDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodMetaDataDownload("
                    << "error=" << data.error
                    << ", errorMessage=" << data.errorMessage
                    << ", url=" << data.url
                    << ", playlist=" << data.playlist
                    << ", userData=" << data.userData
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const VMFileDownload& value)
{
    const VMFileDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodFileDownload("
                    << "progress=" << data.progress
                    << ", filePath=" << data.filePath
                    << ", fileSize=" << data.fileSize
                    << ", playlistIndex=" << data.playlistIndex
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const VMPlaylistDownload& value)
{
    const VMPlaylistDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMPlaylistDownload("
                    << "url=" << data.playlist.webPageUrl()
                    << ", #files=" << data.playlist.vods()
                    << ", progress=" << data.progress
                    << ", fileSize=" << data.fileSize
                    << ", error=" << data.error
                    << ", message=" << data.errorMessage
                    << ", started=" << data.timeStarted
                    << ", changed=" << data.timeChanged
                    << ", format=" << data.format
                    << ", userData=" << data.userData
                    << ")";
    return debug;
}
