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

#include "VMPlaylist.h"

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

VMVideoFormat::VMVideoFormat()
    : d(new VMVideoFormatData())
{
}

bool VMVideoFormat::isValid() const
{
    return !d->id.isEmpty() &&
           !d->streamUrl.isEmpty();
}

/////////////////////////////////////////////////////////////////////////////

VMAudioFormat::VMAudioFormat()
    : d(new VMAudioFormatData())
{
}

bool VMAudioFormat::isValid() const
{
    return !d->id.isEmpty() &&
           !d->streamUrl.isEmpty();
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
    return (d->videoFormats.size() > 0 || d->avFormats.size() > 0) &&
            d->vods.size() > 0 &&
            d->description.isValid();
}

QVariant VMVodPlaylist::videoFormat(int index) const
{
    if (index >= 0 && index < d->videoFormats.size()) {
        return QVariant::fromValue(d->videoFormats[index]);
    }

    return QVariant();
}

QVariant VMVodPlaylist::audioFormat(int index) const
{
    if (index >= 0 && index < d->audioFormats.size()) {
        return QVariant::fromValue(d->audioFormats[index]);
    }

    return QVariant();
}

QVariant VMVodPlaylist::avFormat(int index) const
{
    if (index >= 0 && index < d->avFormats.size()) {
        return QVariant::fromValue(d->avFormats[index]);
    }

    return QVariant();
}

QVariant VMVodPlaylist::vod(int index) const
{
    if (index >= 0 && index < d->vods.size()) {
        return QVariant::fromValue(d->vods[index]);
    }

    return QVariant();
}

int VMVodPlaylist::duration() const
{
    const auto& v = d->vods;
    int duration = 0;
    for (auto i = 0; i < v.size(); ++i) {
        duration += v[i].duration();
    }

    return duration;
}

////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &stream, const VMVideoFormatData &value)
{
    stream << Version;
    stream << value.width;
    stream << value.height;
    stream << value.id;
    stream << value.displayName;
    stream << value.extension;
    stream << value.format;
    stream << value.streamUrl;
    stream << value.codec;
    stream << value.tbr;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMVideoFormatData &value)
{
    quint8 version;
    stream >> version;
    switch (version) {
    case 1:
        stream >> value.width;
        stream >> value.height;
        stream >> value.id;
        stream >> value.displayName;
        stream >> value.extension;
        stream >> value.format;
        stream >> value.streamUrl;
        stream >> value.codec;
        stream >> value.tbr;
        break;
    }

    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMVideoFormat &value)
{
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMVideoFormat &value)
{
    return stream >> value.data();
}

QDataStream &operator<<(QDataStream &stream, const VMAudioFormatData &value)
{
    stream << Version;
    stream << value.id;
    stream << value.displayName;
    stream << value.extension;
    stream << value.streamUrl;
    stream << value.codec;
    stream << value.abr;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, VMAudioFormatData &value)
{
    quint8 version;
    stream >> version;
    switch (version) {
    case 1:
        stream >> value.id;
        stream >> value.displayName;
        stream >> value.extension;
        stream >> value.streamUrl;
        stream >> value.codec;
        stream >> value.abr;
        break;
    }

    return stream;
}

QDataStream &operator<<(QDataStream &stream, const VMAudioFormat &value)
{
    return stream << value.data();
}

QDataStream &operator>>(QDataStream &stream, VMAudioFormat &value)
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
    stream << value.videoFormats;
    stream << value.audioFormats;
    stream << value.avFormats;
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
        stream >> value.videoFormats;
        stream >> value.audioFormats;
        stream >> value.avFormats;
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
                    << "#av=" << data.avFormats.size()
                    << ", #video=" << data.videoFormats.size()
                    << ", #audio=" << data.audioFormats.size()
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

QDebug operator<<(QDebug debug, const VMVideoFormat& value)
{
    const VMVideoFormatData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMVideoFormat("
                    << "name=" << data.displayName
                    << ", id=" << data.id
                    << ", format=" << data.format
                    << ", width=" << data.width
                    << ", height=" << data.height
                    << ", tbr=" << data.tbr
                    << ", codec=" << data.codec
                    << ", ext=" << data.extension
                    << ", streamUrl=" << data.streamUrl
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const VMAudioFormat& value)
{
    const VMAudioFormatData& data = value.data();
    QDebugStateSaver saver(debug);
    debug.nospace() << "VMAudioFormat("
                    << "name=" << data.displayName
                    << ", id=" << data.id
                    << ", abr=" << data.abr
                    << ", codec=" << data.codec
                    << ", ext=" << data.extension
                    << ", streamUrl=" << data.streamUrl
                    << ")";
    return debug;
}

