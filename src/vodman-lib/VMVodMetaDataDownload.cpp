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

#include "VMVodMetaDataDownload.h"

#include <QDebug>
#include <QDebugStateSaver>
#include <QDataStream>

namespace
{
const quint8 Version = 1;
}

VMVodMetaDataDownload::VMVodMetaDataDownload()
    : d(new VMVodMetaDataDownloadData())
{
}


QDataStream &operator<<(QDataStream &stream, const VMVodMetaDataDownloadData &value) {
    stream << Version;
    stream << value.playlist;
    stream << value.errorMessage;
    stream << value.error;
    stream << value.url;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodMetaDataDownloadData &value) {
    quint8 version;
    stream >> version;
    switch (version) {
    case 1:
        stream >> value.playlist;
        stream >> value.errorMessage;
        stream >> value.error;
        stream >> value.url;
        break;
    }

    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMVodMetaDataDownload &value) {
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMVodMetaDataDownload &value) {
    return stream >> value.data();
}


QDebug operator<<(QDebug debug, const VMVodMetaDataDownload& value) {
    const VMVodMetaDataDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodMetaDataDownload("
                    << "error=" << data.error
                    << ", errorMessage=" << data.errorMessage
                    << ", url=" << data.url
                    << ", playlist=" << data.playlist
                    << ")";
    return debug;
}
