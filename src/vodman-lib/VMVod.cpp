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

#include "VMVod.h"

#include <QDebug>
#include <QDebugStateSaver>
#include <QDataStream>

namespace
{
const quint8 Version = 1;
}

VMVodEnums::~VMVodEnums()
{

}

VMVodEnums::VMVodEnums(QObject* parent)
    : QObject(parent)
{}


/////////////////////////////////////////////////////////////////////////////

VMVodFormat::VMVodFormat()
    : d(new VMVodFormatData())
{

}

bool VMVodFormat::isValid() const
{
    return !d->id.isEmpty() &&
            !d->vodUrl.isEmpty();
}

////////////////////////////////////////////////////////////////////

VMVodDescription::VMVodDescription()
    : d(new VMVodDescriptionData())
{

}

bool VMVodDescription::isValid() const
{
    return !d->id.isEmpty();
}

////////////////////////////////////////////////////////////////////


VMVod::VMVod()
    : d(new VMVodData())
{
}

bool VMVod::isValid() const
{
    return d->durationS > 0;
}

////////////////////////////////////////////////////////////////////////////////

VMVodPlaylist::VMVodPlaylist()
    : d(new VMVodPlaylistData())
{
}

bool VMVodPlaylist::isValid() const
{
    return d->formats.size() > 0 &&
            d->vods.size() > 0 &&
            d->description.isValid();
}

QVariant VMVodPlaylist::format(int index) const
{
    return QVariant::fromValue(d->formats[index]);
}

QVariant VMVodPlaylist::vod(int index) const
{
    return QVariant::fromValue(d->vods[index]);
}

////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &stream, const VMVodFormatData &value)
{
    stream << Version;
    stream << value.width;
    stream << value.height;
    stream << value.id;
    stream << value.displayName;
    stream << value.fileExtension;
    stream << value.vodUrl;
    stream << value.format;
    stream << value.fileUrl;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodFormatData &value)
{
    quint8 version;
    stream >> version;
    switch (version) {
    case 1:
        stream >> value.width;
        stream >> value.height;
        stream >> value.id;
        stream >> value.displayName;
        stream >> value.fileExtension;
        stream >> value.vodUrl;
        stream >> value.format;
        stream >> value.fileUrl;
        break;
    }

    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMVodFormat &value)
{
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMVodFormat &value)
{
    return stream >> value.data();
}

QDataStream &operator<<(QDataStream &stream, const VMVodDescriptionData &value)
{
    stream << Version;
    stream << value.thumbnailUrl;
    stream << value.webPageUrl;
    stream << value.title;
    stream << value.fullTitle;
    stream << value.durationS;
    stream << value.id;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodDescriptionData &value)
{
    quint8 version;
    stream >> version;
    switch (version) {
    case 1:
        stream >> value.thumbnailUrl;
        stream >> value.webPageUrl;
        stream >> value.title;
        stream >> value.fullTitle;
        stream >> value.durationS;
        stream >> value.id;
        break;
    }

    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMVodDescription &value)
{
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMVodDescription &value)
{
    return stream >> value.data();
}

QDataStream &operator<<(QDataStream &stream, const VMVodData &value)
{
    stream << Version;
    stream << value.durationS;
    stream << value.playlistIndex;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodData &value)
{
    quint8 version;
    stream >> version;
    switch (version) {
    case 1:
        stream >> value.durationS;
        stream >> value.playlistIndex;
        break;
    }
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMVod &value)
{
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMVod &value)
{
    return stream >> value.data();
}

QDataStream &operator<<(QDataStream &stream, const VMVodPlaylistData &value)
{
    stream << Version;
    stream << value.description;
    stream << value.formats;
    stream << value.vods;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVodPlaylistData &value)
{
    quint8 version;
    stream >> version;
    switch (version) {
    case 1:
        stream >> value.description;
        stream >> value.formats;
        stream >> value.vods;
        break;
    }
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMVodPlaylist &value)
{
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMVodPlaylist &value)
{
    return stream >> value.data();
}

QDebug operator<<(QDebug debug, const VMVodPlaylist& value)
{
    const VMVodPlaylistData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodPlaylist("
                    << "#formats=" << data.formats.size()
                    << ", #vods=" << data.vods.size()
                    << ", desc=" << data.description
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const VMVod& value)
{
    const VMVodData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVod("
                    << "durationS=" << data.durationS
                    << ", playlistIndex=" << data.playlistIndex
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const VMVodDescription& value)
{
    const VMVodDescriptionData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodDescription("
                    << "id=" << data.id
                    << ", title=" << data.title
                    << ", fullTitle=" << data.fullTitle
                    << ", thumbnail=" << data.thumbnailUrl
                    << ", webpageUrl=" << data.webPageUrl
                    << ", durationS=" << data.durationS
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const VMVodFormat& value)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVodFormat("
                    << "name=" << value.displayName()
                    << ", id=" << value.id()
                    << ", format=" << value.format()
                    << ", width=" << value.width()
                    << ", height=" << value.height()
                    << ", tbr=" << value.tbr()
                    << ", fileUrl=" << value.fileUrl()
                    << ", vodUrl=" << value.vodUrl()
                    << ", ext=" << value.fileExtension()
                    << ")";
    return debug;
}

