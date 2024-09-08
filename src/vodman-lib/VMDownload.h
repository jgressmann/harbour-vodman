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

#pragma once

#include "VMPlaylist.h"
#include <QDateTime>
#include <QSet>


struct VMMetaDataDownloadData : public QSharedData
{
    VMPlaylist playlist;
    QString errorMessage;
    QString url;
    QVariant userData;
    VMVodEnums::Error error;

    VMMetaDataDownloadData();
};


class VMMetaDataDownload
{
    Q_GADGET
public:
    Q_PROPERTY(VMVodEnums::Error error READ error CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)
    Q_PROPERTY(VMPlaylist playlist READ playlist CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(QVariant userData READ userData CONSTANT)

public:
    ~VMMetaDataDownload() = default;
    VMMetaDataDownload();
    VMMetaDataDownload(const VMMetaDataDownload& /*other*/) = default;
    VMMetaDataDownload& operator=(const VMMetaDataDownload& /*other*/) = default;

    // true implies error == 0
    bool isValid() const;
    inline VMPlaylist playlist() const { return d->playlist; }
    inline VMVodEnums::Error error() const { return (VMVodEnums::Error)d->error; }
    inline QString errorMessage() const { return d->errorMessage; }
    inline QString url() const { return d->url; }
    inline QVariant userData() const { return d->userData; }

public:
    inline VMMetaDataDownloadData& data() { return *d; }
    inline const VMMetaDataDownloadData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMMetaDataDownloadData> d;
};



struct VMPlaylistDownloadRequest
{
    QString filePath;
    VMPlaylist playlist;
    QString format;
    QVariant userData;
    QVector<int> indices;

    bool isValid() const;
};


struct VMFileDownloadData : public QSharedData
{
    QSet<QString> fileArtifacts;
    QString filePath;
    quint64 fileSize;
    float progress;
    int playlistIndex;

    VMFileDownloadData();
};

class VMFileDownload
{
    Q_GADGET
    Q_PROPERTY(float progress READ progress CONSTANT)
    Q_PROPERTY(QString filePath READ filePath CONSTANT)
    Q_PROPERTY(quint64 fileSize READ fileSize CONSTANT)
    Q_PROPERTY(int playlistIndex READ playlistIndex CONSTANT)

public:
    ~VMFileDownload() = default;
    VMFileDownload();
    VMFileDownload(const VMFileDownload& /*other*/) = default;
    VMFileDownload& operator=(const VMFileDownload& /*other*/) = default;

    bool isValid() const;
    inline float progress() const { return d->progress; }
    inline QString filePath() const { return d->filePath; }
    inline quint64 fileSize() const { return d->fileSize; }
    inline int playlistIndex() const { return d->playlistIndex; }

public:
    inline VMFileDownloadData& data() { return *d; }
    inline const VMFileDownloadData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMFileDownloadData> d;
};



struct VMPlaylistDownloadData : public QSharedData
{
    VMPlaylist playlist;
    QVector<VMFileDownload> files;
    QString errorMessage;
    QString format;
    QDateTime timeStarted;
    QDateTime timeChanged;
    QVariant userData;
    quint64 fileSize;
    float progress;
    VMVodEnums::Error error;
    int currentFileIndex;

    VMPlaylistDownloadData();
};

class VMPlaylistDownload
{
    Q_GADGET
    Q_PROPERTY(float progress READ progress CONSTANT)
    Q_PROPERTY(VMVodEnums::Error error READ error CONSTANT)
    Q_PROPERTY(QString format READ format CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)
    Q_PROPERTY(QDateTime timeStarted READ timeStarted CONSTANT)
    Q_PROPERTY(QDateTime timeChanged READ timeChanged CONSTANT)
    Q_PROPERTY(int currentFileIndex READ currentFileIndex CONSTANT)
    Q_PROPERTY(int files READ files CONSTANT)
    Q_PROPERTY(VMPlaylist playlist READ playlist CONSTANT)
    Q_PROPERTY(quint64 fileSize READ fileSize CONSTANT)
    Q_PROPERTY(QVariant userData READ userData CONSTANT)

public:
    ~VMPlaylistDownload() = default;
    VMPlaylistDownload();
    VMPlaylistDownload(const VMPlaylistDownload& /*other*/) = default;
    VMPlaylistDownload& operator=(const VMPlaylistDownload& /*other*/) = default;

    // true implies error == 0
    bool isValid() const;
    inline float progress() const { return d->progress; }
    inline VMVodEnums::Error error() const { return d->error; }
    inline QString errorMessage() const { return d->errorMessage; }
    inline QDateTime timeStarted() const { return d->timeStarted; }
    inline QDateTime timeChanged() const { return d->timeChanged; }
    inline int files() const { return d->files.size(); }
    Q_INVOKABLE QVariant file(int index) const;
    inline QString format() const { return d->format; }
    inline int currentFileIndex() const { return d->currentFileIndex; }
    inline VMPlaylist playlist() const { return d->playlist; }
    inline quint64 fileSize() const { return d->fileSize; }
    inline const QVector<VMFileDownload>& _files() const { return d->files; }
    inline QVariant userData() const { return d->userData; }

public:
    inline VMPlaylistDownloadData& data() { return *d; }
    inline const VMPlaylistDownloadData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMPlaylistDownloadData> d;
};

Q_DECLARE_METATYPE(VMMetaDataDownload)
Q_DECLARE_METATYPE(VMFileDownload)
Q_DECLARE_METATYPE(VMPlaylistDownload)


QDebug operator<<(QDebug debug, const VMFileDownload& value);
QDebug operator<<(QDebug debug, const VMPlaylistDownload& value);
QDebug operator<<(QDebug debug, const VMMetaDataDownload& value);

