/* The MIT License (MIT)
 *
 * Copyright (c) 2018-2024 Jean Gressmann <jean@0x42.de>
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
class VMPlaylistDownload;
class VMPlaylistDownloadRequest;
class VMMetaDataDownload;

class VMYTDL : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ytdlPath READ ytdlPath WRITE setYtdlPath NOTIFY ytdlPathChanged)
    Q_PROPERTY(int metaDataCacheCapacity READ metaDataCacheCapacity WRITE setMetaDataCacheCapacity NOTIFY metaDataCacheCapacityChanged)
    Q_PROPERTY(int metaDataSecondsValid READ metaDataSecondsValid WRITE setMetaDataSecondsValid NOTIFY metaDataSecondsValidChanged)
    Q_PROPERTY(QStringList customYtdlOptions READ customYtdlOptions WRITE setCustomYtdlOptions NOTIFY customYtdlOptionsChanged)
    Q_PROPERTY(QString cacheDirectory READ cacheDirectory WRITE setCacheDirectory NOTIFY cacheDirectoryChanged)
    Q_PROPERTY(bool ytdlVerbose READ ytdlVerbose WRITE setYtdlVerbose NOTIFY ytdlVerboseChanged)
    Q_PROPERTY(bool usePartFiles READ usePartFiles WRITE setUsePartFiles NOTIFY usePartFilesChanged)
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
    QString cacheDirectory() const { return m_CacheDirectory; }
    void setCacheDirectory(const QString& value);
    bool usePartFiles() const { return m_UsePartFiles; }
    void setUsePartFiles(bool value);

public slots:
    void startFetchMetaData(qint64 token, const QString& url, const QVariant& userData = QVariant());
    void startFetchPlaylist(qint64 token, const VMPlaylistDownloadRequest& request, VMPlaylistDownload* result = nullptr);
    void cancelFetchPlaylist(qint64 token, bool deleteFile);
    void clearCache() { m_MetaDataCache.clear(); }
    QVariantList inProgressPlaylistFetches();

signals:
    void metaDataDownloadCompleted(qint64 id, const VMMetaDataDownload& download);
    void playlistDownloadChanged(qint64 id, const VMPlaylistDownload& download);
    void playlistDownloadCompleted(qint64 id, const VMPlaylistDownload& download);
    void ytdlPathChanged();
    void metaDataCacheCapacityChanged();
    void metaDataSecondsValidChanged();
    void customYtdlOptionsChanged();
    void ytdlVerboseChanged();
    void cacheDirectoryChanged();
    void usePartFilesChanged();

private slots:
    void onMetaDataProcessFinished(int, QProcess::ExitStatus);
    void onProcessError(QProcess::ProcessError);
    void onPlaylistProcessFinished(int, QProcess::ExitStatus);
    void onYoutubeDlVodFileDownloadProcessReadReady();

private:
    Q_DISABLE_COPY(VMYTDL)
    void cleanupProcess(QProcess* process);
    void fillFormatId(VMVideoFormatData& format) const;
    void fillWidth(VMVideoFormatData& format) const;
    void appendVideoFormat(VMVodData& data, const QJsonObject& json) const;
    void appendAudioFormat(VMVodData& data, const QJsonObject& json) const;
    void appendAvFormat(VMVodData& data, const QJsonObject& json) const;
    bool appendVod(VMPlaylistData& data, const QJsonObject& json) const;
    bool available() const;
    QProcess* createProcess();
    static bool parseJson(const QByteArray& bytes, QVector<int>* starts, QVector<int>* ends);

private:
    struct CacheEntry
    {
        VMPlaylist playlist;
        QDateTime fetchTime;
    };

private:
    QCache<QString, CacheEntry> m_MetaDataCache;
    QHash<QProcess*, QVariantMap> m_ProcessMap;
    QHash<qint64, QProcess*> m_VodDownloads;
    Normalizer m_Normalizer;
    QStringList m_CustomOptions;
    QString m_YoutubeDl_Path;
    QString m_CacheDirectory;
    int m_MetaDataSecondsValid;
    bool m_YtdlVerbose;
    bool m_UsePartFiles;
};


