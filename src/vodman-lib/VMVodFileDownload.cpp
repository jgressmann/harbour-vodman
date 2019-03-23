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

#include "VMVodFileDownload.h"


#include <QDebug>
#include <QDebugStateSaver>

bool VMVodPlaylistDownloadRequest::isValid() const
{
    return !filePath.isEmpty() &&
            playlist.isValid() &&
            formatIndex >= 0 &&
            formatIndex < playlist.formats();
}

VMVodFileDownload::VMVodFileDownload()
    : d(new VMVodFileDownloadData())
{}


bool VMVodFileDownload::isValid() const
{
    return !filePath().isEmpty() &&
//           !url().isEmpty() &&
            progress() >= 0;
}

VMVodPlaylistDownload::VMVodPlaylistDownload()
    : d(new VMVodPlaylistDownloadData())
{}


bool VMVodPlaylistDownload::isValid() const
{
    return playlist().isValid() &&
            files() > 0 &&
            currentFileIndex() >= 0 &&
            currentFileIndex() < files();
}

QVariant VMVodPlaylistDownload::file(int index) const
{
    if (index >= 0 && index < d->files.size()) {
        return QVariant::fromValue(d->files[index]);
    }

    return QVariant();
}

QDebug operator<<(QDebug debug, const VMVodFileDownload& value)
{
    const VMVodFileDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodFileDownload("
//                    << "url=" << data.url
                    << "progress=" << data.progress
                    << ", filePath=" << data.filePath
                    << ", fileSize=" << data.fileSize
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const VMVodPlaylistDownload& value)
{
    const VMVodPlaylistDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodPlaylistDownload("
                    << "url=" << data.playlist.description().webPageUrl()
                    << ", #files=" << data.playlist.vods()
                    << ", progress=" << data.progress
                    << ", error=" << data.error
                    << ", message=" << data.errorMessage
                    << ", started=" << data.timeStarted
                    << ", changed=" << data.timeChanged
                    << ", format=" << data.playlist._formats()[data.formatIndex]
                    << ")";
    return debug;
}
