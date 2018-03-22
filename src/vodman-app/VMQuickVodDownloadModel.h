/* The MIT License (MIT)
 *
 * Copyright (c) 2018 Jean Gressmann <jean@0x42.de>
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

#include "service_interface.h" // http://inz.fi/2011/02/18/qmake-and-d-bus/
#include "VMVodFileDownload.h"
#include "VMVodMetaDataDownload.h"

#include <QAbstractListModel>
#include <QMutex>


class VMQuickVodDownload;
class QDBusPendingCallWatcher;
class VMQuickVodDownloadModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool canStartDownload READ canStartDownload NOTIFY canStartDownloadChanged)

public:
    explicit VMQuickVodDownloadModel(QObject *parent = Q_NULLPTR);
    ~VMQuickVodDownloadModel();

public:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QHash<int, QByteArray> roleNames() const;
public:
    Q_INVOKABLE void startDownloadMetaData(const QString& url);
    Q_INVOKABLE void cancelDownloadMetaData();
    Q_INVOKABLE void startDownloadVod(qint64 token, const VMVod& vod, int formatIndex, const QString& filePath);
    Q_INVOKABLE void cancelDownload(int index, bool deleteFile);
    Q_INVOKABLE void cancelDownloads(bool deleteFiles);
    Q_INVOKABLE QString sanatizePath(QString path) const;
    Q_INVOKABLE bool isUrl(QString str) const;
    bool canStartDownload() const;


Q_SIGNALS: // signals
    void metaDataDownloadSubmitted(const QString& url, qint64 token);
    void metaDataDownloadSucceeded(qint64 token, VMVod vod);
    void downloadFailed(QString url, int error, QString filePath);
    void downloadSucceeded(QVariant download);
    void canStartDownloadChanged();

private slots:
    void onVodFileDownloadAdded(qint64 handle, const QByteArray& download);
    void onVodFileDownloadRemoved(qint64 handle, const QByteArray& download);
    void onVodFileDownloadChanged(qint64 handle, const QByteArray& download);
    void onVodFileMetaDataDownloadCompleted(qint64 handle, const QByteArray& download);
    void onStartDownloadVodFileReply(QDBusPendingCallWatcher *self);
    void onNewTokenReply(QDBusPendingCallWatcher *self);
    void onMetaDataDownloadReply(QDBusPendingCallWatcher *self);


private:
    int getHandleRow(qint64 handle) const;
    void vodFileDownloadAdded(qint64 handle, const VMVodFileDownload& download);

private:
    mutable QMutex m_Lock;
    org::duckdns::jgressmann::vodman::service* m_Service;
    QHash<qint64, VMQuickVodDownload*> m_Downloads;
    QList<qint64> m_Rows;
    QList<qint64> m_UserDownloads;
    QString m_Url;
    qint64 m_Token;

private:
    static const QHash<int, QByteArray> ms_Roles;
};
