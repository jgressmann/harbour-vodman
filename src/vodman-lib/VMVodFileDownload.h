#pragma once

#include "VMVod.h"


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

public:
    ~VMVodFileDownload() = default;
    VMVodFileDownload();
    VMVodFileDownload(const VMVodFileDownload& /*other*/) = default;
    VMVodFileDownload& operator=(const VMVodFileDownload& /*other*/) = default;

    inline float progress() const { return d->_progress; }
    inline VMVodEnums::Error error() const { return (VMVodEnums::Error)d->error; }
    inline QString errorMessage() const { return d->errorMessage; }
    inline VMVodFormat format() const { return d->format; }
    inline QString filePath() const { return d->_filePath; }
    inline VMVodDescription description() const { return d->description; }
    bool isValid() const;

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

