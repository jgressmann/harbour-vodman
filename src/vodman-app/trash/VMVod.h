#pragma once

#include <QObject>
#include <QUrl>
#include <QSharedPointer>

class VMVodFormat
{
    Q_GADGET
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(int height READ height CONSTANT)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QUrl url READ url CONSTANT)
    Q_PROPERTY(QString fileExtension READ fileExtension CONSTANT)
    Q_PROPERTY(quint64 fileSize READ fileSize CONSTANT)


public:
//    explicit VMVodFormat(QObject *parent = 0);
//    VMVodFormat(const VMVodFormat& other);
//    VMVodFormat& operator=(const VMVodFormat& other);

    inline int width() const { return _width; }
    inline int height() const { return _height; }
    inline QString id() const { return _id; }
    inline QString displayName() const { return _displayName; }
    inline QString fileExtension() const { return _fileExtension; }
    inline QUrl url() const { return _url; }
    inline quint64 fileSize() const { return _fileSize; }

public:
    int _width;
    int _height;
    QString _id;
    QString _displayName;
    QString _fileExtension;
    QUrl _url;
    quint64 _fileSize;
};

Q_DECLARE_METATYPE(VMVodFormat)

struct VMVodData
{
public:
    QUrl _thumbnail;
    QUrl _webPageUrl;
    QString _title;
    QString _fullTitle;
    QList<VMVodFormat> _formats;
    int durationS;

};

class VMVod
{
    Q_GADGET
    Q_PROPERTY(QUrl thumbnail READ thumbnail CONSTANT)
    Q_PROPERTY(QUrl webPageUrl READ webPageUrl CONSTANT)
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString fullTitle READ fullTitle CONSTANT)
    Q_PROPERTY(QList<VMVodFormat> formats READ formats CONSTANT)

public:
    ~VMVod();
    VMVod();
    VMVod(const VMVod& other);
    VMVod& operator=(const VMVod& other);

public:
    inline QUrl thumbnail() const { return d->_thumbnail; }
    inline QUrl webPageUrl() const { return d->_webPageUrl; }
    inline QString title() const { return d->_title; }
    inline QString fullTitle() const { return d->_fullTitle; }
    inline QList<VMVodFormat> formats() const { return d->_formats; }
    void swap(VMVod& other);

public:
    inline VMVodData& data() { return *d; }

private:
    QSharedPointer<VMVodData> d;
};

Q_DECLARE_METATYPE(VMVod)
