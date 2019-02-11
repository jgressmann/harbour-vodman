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
#include <functional>

#include "VMVod.h"


class QJsonObject;
class VMVodData;
class VMVodFormat;
class VMVodFileDownload;
class VMVodFileDownloadRequest;
class VMVodMetaDataDownload;

class VMYTDL : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ytdlPath READ ytdlPath WRITE setYtdlPath NOTIFY ytdlPathChanged)
public:
    using Normalizer = std::function<void(QString&)>;

public:
    ~VMYTDL();
    VMYTDL(QObject* parent = nullptr);

    QString ytdlPath() const;
    void setYtdlPath(const QString& path);
    Normalizer setUrlNormalizer(Normalizer&& n);

public slots:
    bool startFetchVodMetaData(qint64 token, const QString& url);
    bool startFetchVodFile(qint64 token, const VMVodFileDownloadRequest& request, VMVodFileDownload* result = Q_NULLPTR);
    void cancelFetchVodFile(qint64 token, bool deleteFile);
    QVariantList inProgressVodFetches();

signals:
    void vodMetaDataDownloadCompleted(qint64 id, const VMVodMetaDataDownload& download);
    void vodFileDownloadChanged(qint64 id, const VMVodFileDownload& download);
    void vodFileDownloadCompleted(qint64 id, const VMVodFileDownload& download);
    void ytdlPathChanged();

private slots:
    void onMetaDataProcessFinished(int, QProcess::ExitStatus);
    void onProcessError(QProcess::ProcessError);
    void onVodFileProcessFinished(int, QProcess::ExitStatus);
    void onYoutubeDlVodFileDownloadProcessReadReady();

private:
    Q_DISABLE_COPY(VMYTDL)
    void cleanupProcess(QProcess* process);
    void fillFrameRate(VMVodFormat& format, const QJsonObject& json) const;
    void fillFormatId(VMVodFormat& format, const QJsonObject& json) const;
    bool available() const;

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
    Normalizer m_Normalizer;
    QString m_YoutubeDl_Path;
};


