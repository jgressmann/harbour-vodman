#pragma once

#include "vodman_interface.h"
#include "VMVodFileDownload.h"
#include "VMVodMetaDataDownload.h"

#include <QAbstractListModel>
#include <QMutex>


class VMQuickVodDownload;
//class QDBusVariant;
class QDBusPendingCallWatcher;
class VMQuickVodDownloadModel : public QAbstractListModel
{
    Q_OBJECT
public:


public:
//    Q_PROPERTY(int requestedDownloads READ requestedDownloads NOTIFY requestedDownloadsChanged)
    Q_PROPERTY(bool canStartDownload READ canStartDownload NOTIFY canStartDownloadChanged)
//    Q_PROPERTY(int downloads READ downloads NOTIFY downloadsChanged)

public:
    explicit VMQuickVodDownloadModel(QObject *parent = Q_NULLPTR);
    ~VMQuickVodDownloadModel();

public:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QHash<int, QByteArray> roleNames() const;
    Q_INVOKABLE void startDownloadMetaData(const QString& url);
    Q_INVOKABLE void startDownloadVod(qint64 token, const VMVod& vod, int formatIndex, const QString& filePath);
    Q_INVOKABLE void cancelDownload(int index, bool deleteFile);
    Q_INVOKABLE void cancelDownloads(bool deleteFiles);
//    int requestedDownloads() const;
    bool canStartDownload() const;
//    int downloads() const;


Q_SIGNALS: // signals
//    void connectedChanged(bool connected);
    void metaDataDownloadSubmitted(const QString& url, qint64 token);
    void metaDataDownloadSucceeded(qint64 token, VMVod vod);
    void downloadFailed(QString url, VMVodEnums::Error error);
    void canStartDownloadChanged();
//    void requestedDownloadsChanged();
//    void downloadsChanged();

private slots:
//    void onFetchVodFileStatusChanged(const QVariantList& handles);
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
    org::duckdns::jgressmann::vodman* m_Service;
    QHash<qint64, VMQuickVodDownload*> m_Downloads;
    QList<qint64> m_Rows;
    QString m_Url;
    qint64 m_Token;
//    QHash<qint64, QString> m_PendingMetadataDownloads;
//    QHash<QDBusPendingCallWatcher*, QString> m_UrlsToFetch;

private:
    static const QHash<int, QByteArray> ms_Roles;
};
