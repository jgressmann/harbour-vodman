#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QAtomicInt>
#include "VMVideoUrlFetcher.h"

class VMService : public QObject
{
    Q_OBJECT
public:
    ~VMService();
    explicit VMService(QObject *parent = 0);

public:
    int fetchMetaData(QString url);
    int startFetchVodFile(QString url, QString format, QString filePath);
    QStringList extractors() const;
    int createDownload(const QString &displayName, const QString &applicationIcon, const QString &serviceIcon, const QString &filePath, const QStringList &callback, const QString &cancelMethod, const QString &restartMethod);
    void cancelTransfer(int transferId);
    void clearTransfer(int transferId);
    void clearTransfers();
    void enableNotifications(bool enable);
    QStringList extractors();
    bool notificationsEnabled();
    void startTransfer(int transferId);
    int submitRequest(const QString &displayName, const QString &applicationIcon, const QString &serviceIcon, const QString &filePath, const QString &desiredResolution, const QStringList &callback, const QString &cancelMethod, const QString &restartMethod);

signals:
    void metaDataFetchCompleted(QUrl requestUrl, QVariantMap result);
    void vodStatusChanged(QUrl requestUrl, QVariantMap result);

public slots:

private slots:
//    void failure(QVariantMap result);
//    void success(QVariantMap result);
    void onCompleted(QVariantMap);

private:
    VMVideoUrlFetcher m_Fetcher;
    QHash<QUrl, QVariantMap> m_RequestMap;
    QAtomicInt m_SequenceNumber;
};

