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
#include <QVector>
#include <functional>

#include "VMVod.h"


class QJsonObject;
class VMVodPlaylistDownload;
class VMVodPlaylistDownloadRequest;
class VMVodMetaDataDownload;

class VMYTDL : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ytdlPath READ ytdlPath WRITE setYtdlPath NOTIFY ytdlPathChanged)
    Q_PROPERTY(int metaDataCacheCapacity READ metaDataCacheCapacity WRITE setMetaDataCacheCapacity NOTIFY metaDataCacheCapacityChanged)
    Q_PROPERTY(int metaDataSecondsValid READ metaDataSecondsValid WRITE setMetaDataSecondsValid NOTIFY metaDataSecondsValidChanged)
public:
    using Normalizer = std::function<void(QString&)>;

public:
    ~VMYTDL();
    VMYTDL(QObject* parent = nullptr);

    QString ytdlPath() const;
    void setYtdlPath(const QString& path);
    int metaDataCacheCapacity() const { return m_MetaDataCache.maxCost(); }
    void setMetaDataCacheCapacity(int value);
    int metaDataSecondsValid() const { return m_MetaDataSecondsValid; }
    void setMetaDataSecondsValid(int value);
    Normalizer setUrlNormalizer(Normalizer&& n);

public slots:
    void startFetchVodMetaData(qint64 token, const QString& url);
    void startFetchVodFile(qint64 token, const VMVodPlaylistDownloadRequest& request, VMVodPlaylistDownload* result = nullptr);
    void cancelFetchVodFile(qint64 token, bool deleteFile);
    void clearCache() { m_MetaDataCache.clear(); }
    QVariantList inProgressVodFetches();

signals:
    void vodMetaDataDownloadCompleted(qint64 id, const VMVodMetaDataDownload& download);
    void vodFileDownloadChanged(qint64 id, const VMVodPlaylistDownload& download);
    void vodFileDownloadCompleted(qint64 id, const VMVodPlaylistDownload& download);
    void ytdlPathChanged();
    void metaDataCacheCapacityChanged();
    void metaDataSecondsValidChanged();

private slots:
    void onMetaDataProcessFinished(int, QProcess::ExitStatus);
    void onProcessError(QProcess::ProcessError);
    void onVodFileProcessFinished(int, QProcess::ExitStatus);
    void onYoutubeDlVodFileDownloadProcessReadReady();

private:
    Q_DISABLE_COPY(VMYTDL)
    void cleanupProcess(QProcess* process);
    void fillFormatId(VMVodFormatData& format) const;
    void fillWidth(VMVodFormatData& format) const;
    void appendFormat(VMVodPlaylistData& data, const QString& vodUrl, const QJsonObject& json) const;
    bool available() const;
    QProcess* createProcess();
    static bool parseJson(const QByteArray& bytes, QVector<int>* ends);

private:
    struct CacheEntry
    {
        VMVodPlaylist playlist;
        QDateTime fetchTime;
    };

private:
    QCache<QString, CacheEntry> m_MetaDataCache;
    QHash<QProcess*, QVariantMap> m_ProcessMap;
    QHash<qint64, QProcess*> m_VodDownloads;
    Normalizer m_Normalizer;
    QString m_YoutubeDl_Path;
    int m_MetaDataSecondsValid;
};


