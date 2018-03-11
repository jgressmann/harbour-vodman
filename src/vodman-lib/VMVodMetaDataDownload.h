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

//QDBusArgument& operator<<(QDBusArgument& arg, const VMVodMetaDataDownloadData& value);
//const QDBusArgument& operator>>(const QDBusArgument& arg, VMVodMetaDataDownloadData& value);
//QDBusArgument& operator<<(QDBusArgument& arg, const VMVodMetaDataDownload& value);
//const QDBusArgument& operator>>(const QDBusArgument& arg, VMVodMetaDataDownload& value);

QDebug operator<<(QDebug debug, const VMVodMetaDataDownload& value);

