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

#include "VMService.h"
#include "VMVodFileDownload.h"
#include "VMVodMetaDataDownload.h"
#include "service_adaptor.h" // http://inz.fi/2011/02/18/qmake-and-d-bus/

#include <QDBusConnection>
#include <QDebug>
#include <QDataStream>


VMService::VMService(QObject *parent) : QObject(parent) {
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerObject("/instance", this)) {
        qFatal("Could not register object '/instance': %s", qPrintable(connection.lastError().message()));
    }
    if (!connection.registerService("org.duckdns.jgressmann.vodman.service")) {
        qFatal("DBUS service already taken. Kill the other instance first. %s", qPrintable(connection.lastError().message()));
    }

    new ServiceAdaptor(this);


    connect(    &m_YoutubeDownloader,
                &VMYTDL::fetchVodMetaDataCompleted,
                this,
                &VMService::onFetchVodMetaDataCompleted);
    connect(&m_YoutubeDownloader, &VMYTDL::vodStatusChanged, this, &VMService::onFetchVodFileStatusChanged);
    connect(&m_YoutubeDownloader, &VMYTDL::vodFetchCompleted, this, &VMService::onFetchVodFileCompleted);


    qInfo("Service started at org.duckdns.jgressmann.vodman.service/instance");
}

qint64
VMService::newToken() {
    return m_TokenGenerator++;
}

void
VMService::startFetchVodMetaData(qint64 token, const QString& url) {
    m_YoutubeDownloader.startFetchVodMetaData(token, url);
}

//int
//VMService::startFetchVodMetaData(const QString& url) {
//    return m_YoutubeDownloader.startFetchVodMetaData(url);
//}

//int
//VMService::startFetchVodFile(QString url, QString format, QString filePath) {
//    QVariantMap result;
//    auto id = m_YoutubeDownloader.fetchVod(url, format, filePath, &result);
//    if (id >= 0) {
//        emit vodFileDownloadAdded(id, result);
//    }
//    return id;
//}

void
VMService::startFetchVodFile(qint64 token, const QByteArray& arr) {

    VMVodFileDownloadRequest request;
    {
        QDataStream s(arr);
        s >> request;
    }

    if (request.isValid()) {
        VMVodFileDownload download;
        auto added = m_YoutubeDownloader.fetchVod(token, request, &download);
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
//        id = m_YoutubeDownloader.fetchVod(format, filePath, &download);
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
//    auto id = m_YoutubeDownloader.fetchVod(url, formatId, filePath, &download);
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
//        auto id = m_YoutubeDownloader.fetchVod(format, filePath, &result);
//        if (id >= 0) {
//            emit vodFileDownloadAdded(id, result);
//        }
//        return id;
//    } else {
//        return -1;
//    }
//}

void
VMService::cancelFetchVodFile(qint64 handle, bool deleteFile) {
    m_YoutubeDownloader.cancelFetchVod(handle, deleteFile);
}

QVariantList
VMService::inProgressVodFileFetches() {
    return m_YoutubeDownloader.inProgressVodFetches();
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


