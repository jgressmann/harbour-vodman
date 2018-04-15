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
        VM_1440p, //1440p (2k)	16 Mbps	24 Mbps
        VM_2160p, // 2160p (4k)	35-45 Mbps	53-68 Mbps
    };
    Q_ENUMS(Format)

    enum Error
    {
        VM_ErrorUnknown = -1,
        VM_ErrorNone = 0,
        VM_ErrorServiceUnavailable,
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
    };
    Q_ENUMS(Error)

public:
    ~VMVodEnums();
    explicit VMVodEnums(QObject* parent = Q_NULLPTR);
};

struct VMVodFormatData : public QSharedData
{
    quint64 _fileSize;
    QString _id;
    QString _displayName;
    QString _fileExtension;
    QString _fileUrl;
    QString _vodUrl;
    int _width;
    int _height;
    int _format;
    int _frameRate;

    VMVodFormatData() = default;
};

class VMVodFormat
{
    Q_GADGET
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(int height READ height CONSTANT)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString fileUrl READ fileUrl CONSTANT)
    Q_PROPERTY(QString fileExtension READ fileExtension CONSTANT)
    Q_PROPERTY(quint64 fileSize READ fileSize CONSTANT)
    Q_PROPERTY(VMVodEnums::Format format READ format CONSTANT)
    Q_PROPERTY(int frameRate READ frameRate CONSTANT)

public:
    ~VMVodFormat() = default;
    VMVodFormat();
    VMVodFormat(const VMVodFormat& /*other*/) = default;
    VMVodFormat& operator=(const VMVodFormat& /*other*/) = default;

    inline int width() const { return d->_width; }
    inline int height() const { return d->_height; }
    inline QString id() const { return d->_id; }
    inline QString displayName() const { return d->_displayName; }
    inline QString fileExtension() const { return d->_fileExtension; }
    inline QString fileUrl() const { return d->_fileUrl; }
    inline QString vodUrl() const { return d->_vodUrl; }
    inline quint64 fileSize() const { return d->_fileSize; }
    inline VMVodEnums::Format format() const { return (VMVodEnums::Format)d->_format; }
    inline int frameRate() const { return d->_frameRate; }
    bool isValid() const;

public:
    inline VMVodFormatData& data() { return *d; }
    inline const VMVodFormatData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVodFormatData> d;
};


struct VMVodDescriptionData : public QSharedData
{
    QString _thumbnail;
    QString _webPageUrl;
    QString _title;
    QString _fullTitle;
    QString _id;
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
    inline QString thumbnailUrl() const { return d->_thumbnail; }
    inline QString webPageUrl() const { return d->_webPageUrl; }
    inline QString title() const { return d->_title; }
    inline QString fullTitle() const { return d->_fullTitle; }
    inline QString id() const { return d->_id; }
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
    VMVodDescription description;
    QList<VMVodFormat> _formats;
};

class VMVod
{
    Q_GADGET
    Q_PROPERTY(VMVodDescription description READ description CONSTANT)
    Q_PROPERTY(int formats READ formats CONSTANT)
public:
    ~VMVod() = default;
    VMVod();
    VMVod(const VMVod& /*other*/) = default;
    VMVod& operator=(const VMVod& /*other*/) = default;

public:
    inline VMVodDescription description() const { return d->description; }
    inline const QList<VMVodFormat>& _formats() const { return d->_formats; }
    inline int formats() const { return d->_formats.size(); }
    Q_INVOKABLE QVariant format(int index) const;
    bool isValid() const;

public:
    inline VMVodData& data() { return *d; }
    inline const VMVodData& data() const { return *d; }

private:
    QExplicitlySharedDataPointer<VMVodData> d;
};


Q_DECLARE_METATYPE(VMVodFormat)
Q_DECLARE_METATYPE(VMVodDescription)
Q_DECLARE_METATYPE(VMVod)



QDataStream &operator<<(QDataStream &stream, const VMVodFormatData &value);
QDataStream &operator>>(QDataStream &stream, VMVodFormatData &value);
QDataStream &operator<<(QDataStream &stream, const VMVodFormat &value);
QDataStream &operator>>(QDataStream &stream, VMVodFormat &value);
QDataStream &operator<<(QDataStream &stream, const VMVodDescriptionData &value);
QDataStream &operator>>(QDataStream &stream, VMVodDescriptionData &value);
QDataStream &operator<<(QDataStream &stream, const VMVodDescription &value);
QDataStream &operator>>(QDataStream &stream, VMVodDescription &value);
QDataStream &operator<<(QDataStream &stream, const VMVodData &value);
QDataStream &operator>>(QDataStream &stream, VMVodData &value);
QDataStream &operator<<(QDataStream &stream, const VMVod &value);
QDataStream &operator>>(QDataStream &stream, VMVod &value);


QDebug operator<<(QDebug debug, const VMVodFormat& value);
QDebug operator<<(QDebug debug, const VMVodDescription& value);
QDebug operator<<(QDebug debug, const VMVod& value);

