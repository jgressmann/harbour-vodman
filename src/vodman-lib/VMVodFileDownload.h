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

#pragma once

#include "VMVod.h"
#include <QDateTime>

struct VMVodPlaylistDownloadRequest
{
    QString filePath;
    VMVodPlaylist playlist;
    int formatIndex;

    bool isValid() const;
};


struct VMVodFileDownloadData : public QSharedData
{
//    QString url;
    QString filePath;
    quint64 fileSize;
    float progress;
};

class VMVodFileDownload
{
    Q_GADGET
    Q_PROPERTY(float progress READ progress CONSTANT)
    Q_PROPERTY(QString filePath READ filePath CONSTANT)
    Q_PROPERTY(quint64 fileSize READ fileSize CONSTANT)
//    Q_PROPERTY(QString url READ url CONSTANT)

public:
    ~VMVodFileDownload() = default;
    VMVodFileDownload();
    VMVodFileDownload(const VMVodFileDownload& /*other*/) = default;
    VMVodFileDownload& operator=(const VMVodFileDownload& /*other*/) = default;

    bool isValid() const;
    inline float progress() const { return d->progress; }
    inline QString filePath() const { return d->filePath; }
    inline quint64 fileSize() const { return d->fileSize; }
//    inline QString url() const { return d->url; }

public:
    inline VMVodFileDownloadData& data() { return *d; }
    inline const VMVodFileDownloadData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVodFileDownloadData> d;
};



struct VMVodPlaylistDownloadData : public QSharedData
{
    VMVodPlaylist playlist;
    QList<VMVodFileDownload> files;
    QString errorMessage;
    QDateTime timeStarted;
    QDateTime timeChanged;
    quint64 fileSize;
    float progress;
    int error;
    int formatIndex;
    int currentFileIndex;
};

class VMVodPlaylistDownload
{
    Q_GADGET
    Q_PROPERTY(float progress READ progress CONSTANT)
    Q_PROPERTY(VMVodEnums::Error error READ error CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)
    Q_PROPERTY(QDateTime timeStarted READ timeStarted CONSTANT)
    Q_PROPERTY(QDateTime timeChanged READ timeChanged CONSTANT)
    Q_PROPERTY(int formatIndex READ formatIndex CONSTANT)
    Q_PROPERTY(int currentFileIndex READ currentFileIndex CONSTANT)
    Q_PROPERTY(int files READ files CONSTANT)
    Q_PROPERTY(VMVodPlaylist playlist READ playlist CONSTANT)
    Q_PROPERTY(quint64 fileSize READ fileSize CONSTANT)

public:
    ~VMVodPlaylistDownload() = default;
    VMVodPlaylistDownload();
    VMVodPlaylistDownload(const VMVodPlaylistDownload& /*other*/) = default;
    VMVodPlaylistDownload& operator=(const VMVodPlaylistDownload& /*other*/) = default;

    bool isValid() const;
    inline float progress() const { return d->progress; }
    inline VMVodEnums::Error error() const { return (VMVodEnums::Error)d->error; }
    inline QString errorMessage() const { return d->errorMessage; }
    inline QDateTime timeStarted() const { return d->timeStarted; }
    inline QDateTime timeChanged() const { return d->timeChanged; }
    inline int files() const { return d->files.size(); }
    Q_INVOKABLE QVariant file(int index) const;
    inline int formatIndex() const { return d->formatIndex; }
    inline int currentFileIndex() const { return d->currentFileIndex; }
    inline VMVodPlaylist playlist() const { return d->playlist; }
    inline quint64 fileSize() const { return d->fileSize; }
    inline const QList<VMVodFileDownload>& _files() const { return d->files; }

public:
    inline VMVodPlaylistDownloadData& data() { return *d; }
    inline const VMVodPlaylistDownloadData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVodPlaylistDownloadData> d;
};


Q_DECLARE_METATYPE(VMVodFileDownload)
Q_DECLARE_METATYPE(VMVodPlaylistDownload)




QDebug operator<<(QDebug debug, const VMVodFileDownload& value);
QDebug operator<<(QDebug debug, const VMVodPlaylistDownload& value);

