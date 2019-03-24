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

#include <QObject>
#include <QVariant>
#include <QExplicitlySharedDataPointer>

class QDebug;
class QDataStream;

// need a class that derives from QObject to expose to QML sigh
class VMVodEnums : public QObject
{
    Q_OBJECT
public:
    enum Format
    {
        // https://support.google.com/youtube/answer/1722171?hl=en
        VM_Any = -3,
        VM_Largest = -2,
        VM_Smallest = -1,
        VM_Unknown = 0,
        VM_160p = 1, //
        VM_240p, //
        VM_360p, // 360p	1 Mbps	1.5 Mbps
        VM_480p, // 480p	2.5 Mbps	4 Mbps
        VM_720p, // 720p	5 Mbps	7.5 Mbps
        VM_1080p, // 1080p	8 Mbps	12 Mbps
        VM_1440p, // 1440p (2k)	16 Mbps	24 Mbps
        VM_2160p, // 2160p (4k)	35-45 Mbps	53-68 Mbps
    };
    Q_ENUM(Format)

    enum Error
    {
        VM_ErrorUnknown = -1,
        VM_ErrorNone = 0,
        VM_ErrorNoYoutubeDl,
        VM_ErrorCanceled,
        VM_ErrorCrashed,
        VM_ErrorTimedOut,
        VM_ErrorInvalidResponse,
        VM_ErrorUnsupportedUrl,
        VM_ErrorNoVideo,
        VM_ErrorFormatNotAvailable,
        VM_ErrorNetworkDown,
        VM_ErrorInvalidUrl,
        VM_ErrorNoSpaceLeftOnDevice,
        VM_ErrorAlreadyDownloading,
        VM_ErrorUnsupportedConfigurationFormat,
        VM_ErrorParseError,
        VM_ErrorIo,
        VM_ErrorAccess,
        VM_ErrorContentGone,
        VM_ErrorInvalidRequest
    };
    Q_ENUM(Error)

public:
    ~VMVodEnums();
    explicit VMVodEnums(QObject* parent = Q_NULLPTR);
};

struct VMVideoFormatData : public QSharedData
{
    QString id;
    QString displayName;
    QString extension;
    QString streamUrl;
    QString codec;
    int width;
    int height;
    int format;
    float tbr;

    VMVideoFormatData() = default;
};

class VMVideoFormat
{
    Q_GADGET
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(int height READ height CONSTANT)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString streamUrl READ streamUrl CONSTANT)
    Q_PROPERTY(QString extension READ extension CONSTANT)
    Q_PROPERTY(VMVodEnums::Format format READ format CONSTANT)
    Q_PROPERTY(float tbr READ tbr CONSTANT)

public:
    ~VMVideoFormat() = default;
    VMVideoFormat();
    VMVideoFormat(const VMVideoFormat& /*other*/) = default;
    VMVideoFormat& operator=(const VMVideoFormat& /*other*/) = default;

    inline int width() const { return d->width; }
    inline int height() const { return d->height; }
    inline QString id() const { return d->id; }
    inline QString displayName() const { return d->displayName; }
    inline QString extension() const { return d->extension; }
    inline QString streamUrl() const { return d->streamUrl; }
    inline VMVodEnums::Format format() const { return (VMVodEnums::Format)d->format; }
    inline float tbr() const { return d->tbr; }
    bool isValid() const;

public:
    inline VMVideoFormatData& data() { return *d; }
    inline const VMVideoFormatData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVideoFormatData> d;
};


struct VMAudioFormatData : public QSharedData
{
    QString id;
    QString displayName;
    QString extension;
    QString streamUrl;
    QString codec;
    float abr;

    VMAudioFormatData() = default;
};

class VMAudioFormat
{
    Q_GADGET
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString streamUrl READ streamUrl CONSTANT)
    Q_PROPERTY(QString extension READ extension CONSTANT)
    Q_PROPERTY(QString codec READ codec CONSTANT)
    Q_PROPERTY(float abr READ abr CONSTANT)

public:
    ~VMAudioFormat() = default;
    VMAudioFormat();
    VMAudioFormat(const VMAudioFormat& /*other*/) = default;
    VMAudioFormat& operator=(const VMAudioFormat& /*other*/) = default;

    inline QString id() const { return d->id; }
    inline QString displayName() const { return d->displayName; }
    inline QString streamUrl() const { return d->streamUrl; }
    inline QString extension() const { return d->extension; }
    inline QString codec() const { return d->codec; }
    inline float abr() const { return d->abr; }
    bool isValid() const;

public:
    inline VMAudioFormatData& data() { return *d; }
    inline const VMAudioFormatData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMAudioFormatData> d;
};

struct VMVodDescriptionData : public QSharedData
{
    QString thumbnailUrl;
    QString webPageUrl;
    QString title;
    QString fullTitle;
    QString id;
    int durationS;

    VMVodDescriptionData() = default;
};


class VMVodDescription
{
    Q_GADGET
    Q_PROPERTY(QString thumbnailUrl READ thumbnailUrl CONSTANT)
    Q_PROPERTY(QString webPageUrl READ webPageUrl CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString fullTitle READ fullTitle CONSTANT)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(int duration READ duration CONSTANT)
public:
    ~VMVodDescription() = default;
    VMVodDescription();
    VMVodDescription(const VMVodDescription& /*other*/) = default;
    VMVodDescription& operator=(const VMVodDescription& /*other*/) = default;

public:
    inline QString thumbnailUrl() const { return d->thumbnailUrl; }
    inline QString webPageUrl() const { return d->webPageUrl; }
    inline QString title() const { return d->title; }
    inline QString fullTitle() const { return d->fullTitle; }
    inline QString id() const { return d->id; }
    inline int duration() const { return d->durationS; }
    bool isValid() const;

public:
    inline VMVodDescriptionData& data() { return *d; }
    inline const VMVodDescriptionData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVodDescriptionData> d;
};


struct VMVodData : public QSharedData
{
    int durationS;
    int playlistIndex;

    VMVodData() = default;
};

class VMVod
{
    Q_GADGET
    Q_PROPERTY(int duration READ duration CONSTANT)
    Q_PROPERTY(int playlistIndex READ playlistIndex CONSTANT)
public:
    ~VMVod() = default;
    VMVod();
    VMVod(const VMVod& /*other*/) = default;
    VMVod& operator=(const VMVod& /*other*/) = default;

public:
    int duration() const { return d->durationS; }
    int playlistIndex() const { return d->playlistIndex; }
    bool isValid() const;

public:
    inline VMVodData& data() { return *d; }
    inline const VMVodData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVodData> d;
};

struct VMVodPlaylistData : public QSharedData
{
    VMVodDescription description;
    QList<VMVideoFormat> videoFormats;
    QList<VMAudioFormat> audioFormats;
    QList<VMVideoFormat> avFormats;
    QList<VMVod> vods;
};

class VMVodPlaylist
{
    Q_GADGET
    Q_PROPERTY(VMVodDescription description READ description CONSTANT)
    Q_PROPERTY(int videoFormats READ videoFormats CONSTANT)
    Q_PROPERTY(int audioFormats READ audioFormats CONSTANT)
    Q_PROPERTY(int avFormats READ avFormats CONSTANT)
    Q_PROPERTY(int vods READ vods CONSTANT)
public:
    ~VMVodPlaylist() = default;
    VMVodPlaylist();
    VMVodPlaylist(const VMVodPlaylist& /*other*/) = default;
    VMVodPlaylist& operator=(const VMVodPlaylist& /*other*/) = default;

public:
    inline VMVodDescription description() const { return d->description; }
    inline const QList<VMVideoFormat>& _videoFormats() const { return d->videoFormats; }
    inline const QList<VMAudioFormat>& _audioFormats() const { return d->audioFormats; }
    inline const QList<VMVideoFormat>& _avFormats() const { return d->avFormats; }
    inline const QList<VMVod>& _vods() const { return d->vods; }
    inline int videoFormats() const { return d->videoFormats.size(); }
    inline int audioFormats() const { return d->audioFormats.size(); }
    inline int avFormats() const { return d->avFormats.size(); }
    inline int vods() const { return d->vods.size(); }
    Q_INVOKABLE QVariant videoFormat(int index) const;
    Q_INVOKABLE QVariant audioFormat(int index) const;
    Q_INVOKABLE QVariant avFormat(int index) const;
    Q_INVOKABLE QVariant vod(int index) const;
    bool isValid() const;
    int duration() const;

public:
    inline VMVodPlaylistData& data() { return *d; }
    inline const VMVodPlaylistData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVodPlaylistData> d;
};


Q_DECLARE_METATYPE(VMVideoFormat)
Q_DECLARE_METATYPE(VMAudioFormat)
Q_DECLARE_METATYPE(VMVodDescription)
Q_DECLARE_METATYPE(VMVod)
Q_DECLARE_METATYPE(VMVodPlaylist)


QDataStream &operator<<(QDataStream &stream, const VMAudioFormatData &value);
QDataStream &operator>>(QDataStream &stream, VMAudioFormatData &value);
QDataStream &operator<<(QDataStream &stream, const VMAudioFormat &value);
QDataStream &operator>>(QDataStream &stream, VMAudioFormat &value);
QDataStream &operator<<(QDataStream &stream, const VMVideoFormatData &value);
QDataStream &operator>>(QDataStream &stream, VMVideoFormatData &value);
QDataStream &operator<<(QDataStream &stream, const VMVideoFormat &value);
QDataStream &operator>>(QDataStream &stream, VMVideoFormat &value);
QDataStream &operator<<(QDataStream &stream, const VMVodDescriptionData &value);
QDataStream &operator>>(QDataStream &stream, VMVodDescriptionData &value);
QDataStream &operator<<(QDataStream &stream, const VMVodDescription &value);
QDataStream &operator>>(QDataStream &stream, VMVodDescription &value);
QDataStream &operator<<(QDataStream &stream, const VMVodData &value);
QDataStream &operator>>(QDataStream &stream, VMVodData &value);
QDataStream &operator<<(QDataStream &stream, const VMVod &value);
QDataStream &operator>>(QDataStream &stream, VMVod &value);
QDataStream &operator<<(QDataStream &stream, const VMVodPlaylist &value);
QDataStream &operator>>(QDataStream &stream, VMVodPlaylist &value);


QDebug operator<<(QDebug debug, const VMAudioFormat& value);
QDebug operator<<(QDebug debug, const VMVideoFormat& value);
QDebug operator<<(QDebug debug, const VMVodDescription& value);
QDebug operator<<(QDebug debug, const VMVod& value);
QDebug operator<<(QDebug debug, const VMVodPlaylist& value);
