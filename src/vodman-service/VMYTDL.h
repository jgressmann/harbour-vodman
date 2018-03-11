#pragma once

#include <QCache>
#include <QProcess>
#include <QVariantMap>
#include <QMutex>

#include "VMVod.h"

class QJsonObject;
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

    static void runInitialCheck();
    static bool available() { return _works; }
    static QString version() { return _version_str; }

    bool startFetchVodMetaData(qint64 token, const QString& url);
    bool fetchVod(qint64 token, const VMVodFileDownloadRequest& request, VMVodFileDownload* result = Q_NULLPTR);
    //int fetchVod(const VMVodFormat& format, const QString& filePath, VMVodFileDownload* result = Q_NULLPTR);
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

private:
    QMutex m_Lock;
    QCache<QString, VMVod> m_MetaDataCache;
    QHash<QProcess*, QVariantMap> m_ProcessMap;
    QHash<qint64, QProcess*> m_VodDownloads;

private: // statics
    static QString _version_str;
    static bool _works;
};


