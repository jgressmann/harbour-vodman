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

#pragma once

#include "VMVod.h"
#include <QDateTime>

struct VMVodFileDownloadRequest
{
    QString filePath;
    VMVodFormat format;
    VMVodDescription description;

    bool isValid() const;
};


struct VMVodFileDownloadData : public QSharedData
{
    VMVodDescription description;
    VMVodFormat format;
    QString errorMessage;
    QString _filePath;
    quint64 fileSize;
    QDateTime timeStarted;
    QDateTime timeChanged;
    float _progress;
    int error;
};

class VMVodFileDownload
{
    Q_GADGET
    Q_PROPERTY(float progress READ progress CONSTANT)
    Q_PROPERTY(VMVodEnums::Error error READ error CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)
    Q_PROPERTY(VMVodFormat format READ format CONSTANT)
    Q_PROPERTY(QString filePath READ filePath CONSTANT)
    Q_PROPERTY(VMVodDescription description READ description CONSTANT)
    Q_PROPERTY(quint64 fileSize READ fileSize CONSTANT)
    Q_PROPERTY(QDateTime timeStarted READ timeStarted CONSTANT)
    Q_PROPERTY(QDateTime timeChanged READ timeChanged CONSTANT)

public:
    ~VMVodFileDownload() = default;
    VMVodFileDownload();
    VMVodFileDownload(const VMVodFileDownload& /*other*/) = default;
    VMVodFileDownload& operator=(const VMVodFileDownload& /*other*/) = default;

    bool isValid() const;
    inline float progress() const { return d->_progress; }
    inline VMVodEnums::Error error() const { return (VMVodEnums::Error)d->error; }
    inline QString errorMessage() const { return d->errorMessage; }
    inline VMVodFormat format() const { return d->format; }
    inline QString filePath() const { return d->_filePath; }
    inline VMVodDescription description() const { return d->description; }
    inline quint64 fileSize() const { return d->fileSize; }
    inline QDateTime timeStarted() const { return d->timeStarted; }
    inline QDateTime timeChanged() const { return d->timeChanged; }


public:
    inline VMVodFileDownloadData& data() { return *d; }
    inline const VMVodFileDownloadData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVodFileDownloadData> d;
};

Q_DECLARE_METATYPE(VMVodFileDownload)


QDataStream &operator<<(QDataStream &stream, const VMVodFileDownloadRequest &value);
QDataStream &operator>>(QDataStream &stream, VMVodFileDownloadRequest &value);
QDataStream &operator<<(QDataStream &stream, const VMVodFileDownloadData &value);
QDataStream &operator>>(QDataStream &stream, VMVodFileDownloadData &value);
QDataStream &operator<<(QDataStream &stream, const VMVodFileDownload &value);
QDataStream &operator>>(QDataStream &stream, VMVodFileDownload &value);


QDebug operator<<(QDebug debug, const VMVodFileDownload& value);

