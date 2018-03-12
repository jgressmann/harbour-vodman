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
    Q_INVOKABLE void startDownloadVod(qint64 token, const VMVod& vod, int formatIndex, const QString& filePath);
    Q_INVOKABLE void cancelDownload(int index, bool deleteFile);
    Q_INVOKABLE void cancelDownloads(bool deleteFiles);
    bool canStartDownload() const;


Q_SIGNALS: // signals
    void metaDataDownloadSubmitted(const QString& url, qint64 token);
    void metaDataDownloadSucceeded(qint64 token, VMVod vod);
    void downloadFailed(QString url, int error, QString filePath);
    void downloadSucceeded(QVariant download);
    void canStartDownloadChanged();

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
    QList<qint64> m_UserDownloads;
    QString m_Url;
    qint64 m_Token;

private:
    static const QHash<int, QByteArray> ms_Roles;
};
