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

#include <QCache>
#include <QProcess>
#include <QVariantMap>
#include <QDateTime>
#include <QScopedPointer>

#include "VMVod.h"


class QJsonObject;
class QTemporaryFile;
class VMVodFormat;
class VMVodFileDownload;
class VMVodFileDownloadRequest;
class VMVodMetaDataDownload;
class VMYTDL : public QObject
{
    Q_OBJECT
public:
    ~VMYTDL();
    VMYTDL(QObject* parent = Q_NULLPTR);

    static void initialize();
    static void finalize();
    static bool available() { return !ms_YoutubeDl_Version.isEmpty(); }
    static QString version() { return ms_YoutubeDl_Version; }

    bool startFetchVodMetaData(qint64 token, const QString& url);
    bool fetchVod(qint64 token, const VMVodFileDownloadRequest& request, VMVodFileDownload* result = Q_NULLPTR);
    void cancelFetchVod(qint64 token, bool deleteFile);
    QVariantList inProgressVodFetches();
    int fetchVodYoutubeDl(const VMVodFormat& format, const QString& filePath, VMVodFileDownload* _download);

signals:
    void fetchVodMetaDataCompleted(qint64 id, const VMVodMetaDataDownload& download);
    void vodStatusChanged(qint64 id, const VMVodFileDownload& download);
    void vodFetchCompleted(qint64 id, const VMVodFileDownload& download);

private slots:
    void onMetaDataProcessFinished(int, QProcess::ExitStatus);
    void onProcessError(QProcess::ProcessError);
    void onVodFileProcessFinished(int, QProcess::ExitStatus);
    void onYoutubeDlVodFileDownloadProcessReadReady();

private:
    Q_DISABLE_COPY(VMYTDL)
    QVariantMap parseResponse(QJsonDocument);
    void cleanupProcess(QProcess* process);
    void normalizeUrl(QUrl& url) const;
    void fillFrameRate(VMVodFormat& format, const QJsonObject& json) const;
    void fillFormatId(VMVodFormat& format, const QJsonObject& json) const;
    static bool findExecutable(const QString& name, const QString& path = QString());

private:
    struct CacheEntry
    {
        VMVod vod;
        QDateTime fetchTime;
    };

private:
    QCache<QString, CacheEntry> m_MetaDataCache;
    QHash<QProcess*, QVariantMap> m_ProcessMap;
    QHash<qint64, QProcess*> m_VodDownloads;

private: // statics
    static QString ms_YoutubeDl_Version;
    static QString ms_YoutubeDl_Path;
};


