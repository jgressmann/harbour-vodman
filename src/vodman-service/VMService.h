#pragma once

#include <QObject>
#include <QAtomicInt>
#include "VMYTDL.h"

class VMVodMetaDataDownload;
class VMVodFileDownload;
class VMService : public QObject
{
    Q_OBJECT
public:
    explicit VMService(QObject *parent = Q_NULLPTR);

public slots:
    qint64 newToken();
    //int startFetchVodMetaData(const QString& url);
    void startFetchVodMetaData(qlonglong token, const QString& url);
    void startFetchVodFile(qlonglong token, const QByteArray& vmVodFileDownloadRequest);
    //int startFetchVodFile(const QByteArray& vmVodFormat, const QString& filePath);
//    int startFetchVodFile(const QString& url, const QString& formatId, const QString& filePath);
    void cancelFetchVodFile(qlonglong handle, bool deleteFile);
    QVariantList inProgressVodFileFetches();

signals:
    void vodMetaDataDownloadCompleted(qint64 handle, const QByteArray &result);
//    void fetchVodFileCompleted(int, QVariantMap result);
//    void fetchVodFileStatusChanged(int, QVariantMap result);
    void vodFileDownloadAdded(qlonglong handle, const QByteArray &result);
    void vodFileDownloadChanged(qlonglong handle, const QByteArray &result);
    void vodFileDownloadRemoved(qlonglong handle, const QByteArray &result);
    void foo(const QByteArray &result);

private slots:
    void onFetchVodMetaDataCompleted(qint64, const VMVodMetaDataDownload &result);
    void onFetchVodFileCompleted(qint64, const VMVodFileDownload& result);
    void onFetchVodFileStatusChanged(qint64, const VMVodFileDownload& result);

private:
    QAtomicInteger<qint64> m_TokenGenerator;
    VMYTDL m_YoutubeDownloader;
};

