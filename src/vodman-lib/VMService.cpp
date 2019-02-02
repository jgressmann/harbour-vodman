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

#include "VMService.h"
#include "VMVodFileDownload.h"
#include "VMVodMetaDataDownload.h"
#include "VMYTDL.h"


#include <QDebug>
#include <QDataStream>


VMService::VMService(QObject *parent)
    : QObject(parent)
    , m_YoutubeDownloader(nullptr)
{ }

VMYTDL*
VMService::ytdl() const { return m_YoutubeDownloader; }

void
VMService::setYtdl(VMYTDL* value)
{
    if (value != m_YoutubeDownloader) {
        if (m_YoutubeDownloader) {
            disconnect(m_YoutubeDownloader, &VMYTDL::fetchVodMetaDataCompleted, this, &VMService::onFetchVodMetaDataCompleted);
            disconnect(m_YoutubeDownloader, &VMYTDL::vodStatusChanged, this, &VMService::onFetchVodFileStatusChanged);
            disconnect(m_YoutubeDownloader, &VMYTDL::vodFetchCompleted, this, &VMService::onFetchVodFileCompleted);
        }

        m_YoutubeDownloader = value;

        if (m_YoutubeDownloader) {
            connect(m_YoutubeDownloader, &VMYTDL::fetchVodMetaDataCompleted, this, &VMService::onFetchVodMetaDataCompleted);
            connect(m_YoutubeDownloader, &VMYTDL::vodStatusChanged, this, &VMService::onFetchVodFileStatusChanged);
            connect(m_YoutubeDownloader, &VMYTDL::vodFetchCompleted, this, &VMService::onFetchVodFileCompleted);
        }

        emit ytdlChanged();
    }
}

qint64
VMService::newToken() {
    return m_TokenGenerator++;
}

void
VMService::startFetchVodMetaData(qint64 token, const QString& url) {
    if (!m_YoutubeDownloader) {
        qWarning("ytdl property not set\n");
        return;
    }

    m_YoutubeDownloader->startFetchVodMetaData(token, url);
}

//int
//VMService::startFetchVodMetaData(const QString& url) {
//    return m_YoutubeDownloader->startFetchVodMetaData(url);
//}

//int
//VMService::startFetchVodFile(QString url, QString format, QString filePath) {
//    QVariantMap result;
//    auto id = m_YoutubeDownloader->fetchVod(url, format, filePath, &result);
//    if (id >= 0) {
//        emit vodFileDownloadAdded(id, result);
//    }
//    return id;
//}

void
VMService::startFetchVodFile(qint64 token, const QByteArray& arr)
{
    if (!m_YoutubeDownloader) {
        qWarning("ytdl property not set\n");
        return;
    }

    VMVodFileDownloadRequest request;
    {
        QDataStream s(arr);
        s >> request;
    }

    if (request.isValid()) {
        VMVodFileDownload download;
        auto added = m_YoutubeDownloader->fetchVod(token, request, &download);
        QByteArray b;
        {
            QDataStream s(&b, QIODevice::WriteOnly);
            s << download;
        }
        if (added) {
            emit vodFileDownloadAdded(token, b);
        } else {
            emit vodFileDownloadRemoved(token, b);
        }
    }
}

//int
//VMService::startFetchVodFile(const QByteArray& vmVodFormat, const QString& filePath) {

//    VMVodFormat format;
//    {
//        QDataStream s(vmVodFormat);
//        s >> format;
//    }

//    int id = -1;
//    if (format.isValid()) {
//        VMVodFileDownload download;
//        id = m_YoutubeDownloader->fetchVod(format, filePath, &download);
//        if (id >= 0) {
//            QByteArray b;
//            {
//                QDataStream s(&b, QIODevice::WriteOnly);
//                s << download;
//            }
//            emit vodFileDownloadAdded(id, b);
//        }
//    }

//    return id;
//}

//int
//VMService::startFetchVodFile(const QString& url, const QString& formatId, const QString& filePath)
//{
//    VMVodFileDownload download;
//    auto id = m_YoutubeDownloader->fetchVod(url, formatId, filePath, &download);
//    if (id >= 0) {
//        QByteArray b;
//        {
//            QDataStream s(&b, QIODevice::WriteOnly);
//            s << download;
//        }
//        emit vodFileDownloadAdded(id, b);
//    }

//    return id;
//}

//int
//VMService::startFetchVodFile(const QVariant& vmVodFormat, const QString& filePath) {
//    VMVodFormat format = qvariant_cast<VMVodFormat>(vmVodFormat);
//    if (format.isValid()) {
//        QVariantMap result;
//        auto id = m_YoutubeDownloader->fetchVod(format, filePath, &result);
//        if (id >= 0) {
//            emit vodFileDownloadAdded(id, result);
//        }
//        return id;
//    } else {
//        return -1;
//    }
//}

void
VMService::cancelFetchVodFile(qint64 handle, bool deleteFile)
{
    if (!m_YoutubeDownloader) {
        qWarning("ytdl property not set\n");
        return;
    }

    m_YoutubeDownloader->cancelFetchVod(handle, deleteFile);
}

QVariantList
VMService::inProgressVodFileFetches() {
    if (!m_YoutubeDownloader) {
        qWarning("ytdl property not set\n");
        return QVariantList();
    }

    return m_YoutubeDownloader->inProgressVodFetches();
}

void
VMService::onFetchVodMetaDataCompleted(qint64 handle, const VMVodMetaDataDownload& result) {
    QByteArray b;
    {
        QDataStream s(&b, QIODevice::WriteOnly);
        s << result;
    }
    emit vodMetaDataDownloadCompleted(handle, b);

//    {
//        b.clear();
//        QDataStream s(&b, QIODevice::WriteOnly);
//        s << QUrl("http://reddit.com");
//    }
//    emit foo(b);
}

void
VMService::onFetchVodFileCompleted(qint64 handle, const VMVodFileDownload& result) {
    QByteArray b;
    {
        QDataStream s(&b, QIODevice::WriteOnly);
        s << result;
    }
    emit vodFileDownloadRemoved(handle, b);
}

void
VMService::onFetchVodFileStatusChanged(qint64 handle, const VMVodFileDownload& result) {
    QByteArray b;
    {
        QDataStream s(&b, QIODevice::WriteOnly);
        s << result;
    }
    emit vodFileDownloadChanged(handle, b);
}


