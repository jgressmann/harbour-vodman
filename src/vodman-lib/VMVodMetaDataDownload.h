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

struct VMVodMetaDataDownloadData : public QSharedData
{
    VMVod _vod;
    QString errorMessage;
    QString _url;
    int error;

    VMVodMetaDataDownloadData() = default;
};

class VMVodMetaDataDownload
{
    Q_GADGET
public:
    Q_PROPERTY(VMVodEnums::Error error READ error CONSTANT)
    Q_PROPERTY(QString errorMessage READ errorMessage CONSTANT)
    Q_PROPERTY(VMVod vod READ vod CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)

public:
    ~VMVodMetaDataDownload() = default;
    VMVodMetaDataDownload();
    VMVodMetaDataDownload(const VMVodMetaDataDownload& /*other*/) = default;
    VMVodMetaDataDownload& operator=(const VMVodMetaDataDownload& /*other*/) = default;

    inline VMVod vod() const { return d->_vod; }
    inline VMVodEnums::Error error() const { return (VMVodEnums::Error)d->error; }
    inline QString errorMessage() const { return d->errorMessage; }
    inline QString url() const { return d->_url; }
    inline bool isValid() const { return d->_vod.isValid(); }

public:
    inline VMVodMetaDataDownloadData& data() { return *d; }
    inline const VMVodMetaDataDownloadData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVodMetaDataDownloadData> d;
};

Q_DECLARE_METATYPE(VMVodMetaDataDownload)


QDataStream &operator<<(QDataStream &stream, const VMVodMetaDataDownloadData &value);
QDataStream &operator>>(QDataStream &stream, VMVodMetaDataDownloadData &value);
QDataStream &operator<<(QDataStream &stream, const VMVodMetaDataDownload &value);
QDataStream &operator>>(QDataStream &stream, VMVodMetaDataDownload &value);

QDebug operator<<(QDebug debug, const VMVodMetaDataDownload& value);

