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

#include "VMPlaylist.h"


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
    Q_PROPERTY(QStringList customYtdlOptions READ customYtdlOptions WRITE setCustomYtdlOptions NOTIFY customYtdlOptionsChanged)
    Q_PROPERTY(bool ytdlVerbose READ ytdlVerbose WRITE setYtdlVerbose NOTIFY ytdlVerboseChanged)
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
    QStringList customYtdlOptions() const { return m_CustomOptions; }
    void setCustomYtdlOptions(const QStringList& value);
    bool ytdlVerbose() const { return m_YtdlVerbose; }
    void setYtdlVerbose(bool value);

public slots:
    void startFetchMetaData(qint64 token, const QString& url, const QVariant& userData = QVariant());
    void startFetchPlaylist(qint64 token, const VMVodPlaylistDownloadRequest& request, VMVodPlaylistDownload* result = nullptr);
    void cancelFetchPlaylist(qint64 token, bool deleteFile);
    void clearCache() { m_MetaDataCache.clear(); }
    QVariantList inProgressPlaylistFetches();

signals:
    void metaDataDownloadCompleted(qint64 id, const VMVodMetaDataDownload& download);
    void playlistDownloadChanged(qint64 id, const VMVodPlaylistDownload& download);
    void playlistDownloadCompleted(qint64 id, const VMVodPlaylistDownload& download);
    void ytdlPathChanged();
    void metaDataCacheCapacityChanged();
    void metaDataSecondsValidChanged();
    void customYtdlOptionsChanged();
    void ytdlVerboseChanged();

private slots:
    void onMetaDataProcessFinished(int, QProcess::ExitStatus);
    void onProcessError(QProcess::ProcessError);
    void onVodPlaylistProcessFinished(int, QProcess::ExitStatus);
    void onYoutubeDlVodFileDownloadProcessReadReady();

private:
    Q_DISABLE_COPY(VMYTDL)
    void cleanupProcess(QProcess* process);
    void fillFormatId(VMVideoFormatData& format) const;
    void fillWidth(VMVideoFormatData& format) const;
    void appendVideoFormat(VMVodPlaylistData& data, const QJsonObject& json) const;
    void appendAudioFormat(VMVodPlaylistData& data, const QJsonObject& json) const;
    void appendAvFormat(VMVodPlaylistData& data, const QJsonObject& json) const;
    bool available() const;
    QProcess* createProcess();
    static bool parseJson(const QByteArray& bytes, QVector<int>* starts, QVector<int>* ends);

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
    QStringList m_CustomOptions;
    QString m_YoutubeDl_Path;
    int m_MetaDataSecondsValid;
    bool m_YtdlVerbose;
};


