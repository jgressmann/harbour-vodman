/* The MIT License (MIT)
 *
 * Copyright (c) 2016 Jean Gressmann <jean@0x42.de>
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
#include <QDataStream>



bool VMVodFileDownloadRequest::isValid() const {
    return !filePath.isEmpty() && format.isValid() && description.isValid();
}


QDataStream &operator<<(QDataStream &stream, const VMVodFileDownloadRequest &value) {
    stream << value.description;
    stream << value.filePath;
    stream << value.format;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodFileDownloadRequest &value) {
    stream >> value.description;
    stream >> value.filePath;
    stream >> value.format;
    return stream;
}

VMVodFileDownload::VMVodFileDownload()
    : d(new VMVodFileDownloadData())
{
}


bool VMVodFileDownload::isValid() const {
    return d->error == VMVodEnums::VM_ErrorNone &&
            d->format.isValid() &&
            d->description.isValid();
}

QDataStream &operator<<(QDataStream &stream, const VMVodFileDownloadData &value) {
    stream << value.format;
    stream << value.errorMessage;
    stream << value._progress;
    stream << value.error;
    stream << value._filePath;
    stream << value.description;
    stream << value.fileSize;
    stream << value.timeStarted;
    stream << value.timeChanged;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodFileDownloadData &value) {
    stream >> value.format;
    stream >> value.errorMessage;
    stream >> value._progress;
    stream >> value.error;
    stream >> value._filePath;
    stream >> value.description;
    stream >> value.fileSize;
    stream >> value.timeStarted;
    stream >> value.timeChanged;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMVodFileDownload &value) {
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMVodFileDownload &value) {
    return stream >> value.data();
}


QDebug operator<<(QDebug debug, const VMVodFileDownload& value) {
    const VMVodFileDownloadData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodFileDownload("
//                    << "url=" << value.url()
                    << "progress=" << data._progress
                    << ", error=" << data.error
                    << ", message=" << data.errorMessage
                    << ", filePath=" << data._filePath
                    << ", fileSize=" << data.fileSize
                    << ", started=" << data.timeStarted
                    << ", changed=" << data.timeChanged
                    << ", format=" << data.format
                    << ", desc=" << data.description
                    << ")";
    return debug;
}
