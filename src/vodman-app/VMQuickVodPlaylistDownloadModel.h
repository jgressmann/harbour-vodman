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


#include "VMDownload.h"


#include <QAbstractListModel>
#include <QNetworkConfigurationManager>


class VMQuickVodPlaylistDownload;
class VMPlaylistDownload;
class VMMetaDataDownload;
class VMYTDL;
class VMQuickVodPlaylistDownloadModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY isOnlineChanged)
    Q_PROPERTY(bool isOnBroadband READ isOnBroadband NOTIFY isOnBroadbandChanged)
    Q_PROPERTY(bool isOnMobile READ isOnMobile NOTIFY isOnMobileChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool metaDataDownloadsPending READ metaDataDownloadsPending NOTIFY metaDataDownloadsPendingChanged)
    Q_PROPERTY(VMYTDL* ytdl READ ytdl WRITE setYtdl NOTIFY ytdlChanged)

public:
    explicit VMQuickVodPlaylistDownloadModel(QObject *parent = Q_NULLPTR);
    ~VMQuickVodPlaylistDownloadModel();

public:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QHash<int, QByteArray> roleNames() const;

public:
    Q_INVOKABLE qint64 newToken();
    Q_INVOKABLE void startDownloadMetaData(qint64 token, const QString& url, const QVariant& userData = QVariant());
    Q_INVOKABLE void startDownloadPlaylist(qint64 token, const VMPlaylist& playlist, QString format, const QString& filePath, const QVariant& userData = QVariant());
    Q_INVOKABLE void cancelDownload(int index, bool deleteFile);
    Q_INVOKABLE void cancelDownloads(bool deleteFiles);
    Q_INVOKABLE bool isUrl(QString str) const;
    bool isOnline() const;
    bool isOnBroadband() const;
    bool isOnMobile() const;
    bool busy() const;
    bool metaDataDownloadsPending() const { return m_MetaDataDownloads > 0; }
    VMYTDL* ytdl() const { return m_Ytdl; }
    void setYtdl(VMYTDL* value);

Q_SIGNALS:
    void metaDataDownloadSubmitted(const QString& url, qint64 token);
    void metaDataDownloadSucceeded(qint64 token, VMPlaylist playlist);
    void downloadFailed(QString url, int error, QString filePath);
    void downloadSucceeded(QVariant download);
    void isOnlineChanged();
    void isOnBroadbandChanged();
    void isOnMobileChanged();
    void busyChanged();
    void metaDataDownloadsPendingChanged();
    void ytdlChanged();

private slots:
    void onPlaylistDownloadCompleted(qint64 handle, const VMPlaylistDownload& download);
    void onPlaylistDownloadChanged(qint64 handle, const VMPlaylistDownload& download);
    void onMetaDataDownloadCompleted(qint64 handle, const VMMetaDataDownload& download);
    void onOnlineChanged(bool online);

private:
    int getHandleRow(qint64 handle) const;
    void playlistDownloadAdded(qint64 handle, const VMPlaylistDownload& download);

private:
    QNetworkConfigurationManager m_NetworkConfigurationManager;
    QHash<qint64, VMQuickVodPlaylistDownload*> m_Downloads;
    QList<qint64> m_Rows;
    QHash<qint64, QString> m_UrlsBeingDownloaded;
    VMYTDL* m_Ytdl;
    qint64 m_TokenGenerator;
    int m_MetaDataDownloads;

private:
    static const QHash<int, QByteArray> ms_Roles;
};

