#include "VMService.h"
#include "vodman_adaptor.h"

#include <QDBusConnection>
#include <transferengineinterface.h>

VMService::~VMService() {

}

VMService::VMService(QObject *parent) : QObject(parent), m_SequenceNumber(0) {
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerObject("/org/duckdns/jgressmann/vodman", this)) {
        qFatal("Could not register object \'/org/duckdns/jgressmann/vodman\'");
    }
    if (!connection.registerService("org.duckdns.jgressmann.vodman")) {
        qFatal("DBUS service already taken. Kill the other instance first.");
    }

    new VMServiceAdaptor(this);

//    connect(&m_Fetcher, SIGNAL(success(QVariantMap), this, SLOT(success(QVariantMap));
//    connect(&m_Fetcher, SIGNAL(failure(QVariantMap)), this, SLOT(failure(QVariantMap));
    connect(&m_Fetcher, SIGNAL(success(QVariantMap)), this, SLOT(onCompleted(QVariantMap)));
    connect(&m_Fetcher, SIGNAL(failure(QVariantMap)), this, SLOT(onCompleted(QVariantMap)));
}

void VMService::onCompleted(QVariantMap m) {

    emit metaDataFetchCompleted(qvariant_cast<QUrl>(m["request_url"]).toString(), m);
}

//void VMService::failure(QVariantMap result) {

//}

//void VMService::success(QVariantMap result) {

//}

int
VMService::fetchMetaData(QString url) {
    int id = m_SequenceNumber++;
    m_Fetcher.fetchMetaData(id, url);
    return id;
}

int
VMService::startFetchVodFile(QString url, QString format, QString filePath) {
    int id = m_SequenceNumber++;
    m_Fetcher.fetchVod(id, url, format, filePath);
    return id;
}

QStringList
VMService::extractors() {
    return QStringList() << "foo" << "bar";
}

int
VMService::createDownload(const QString &displayName, const QString &applicationIcon, const QString &serviceIcon, const QString &filePath, const QStringList &callback, const QString &cancelMethod, const QString &restartMethod) {
    return 0;
}


void VMService::cancelTransfer(int transferId) {

}

void VMService::clearTransfer(int transferId) {

}

void VMService::clearTransfers() {

}

void VMService::enableNotifications(bool enable) {

}

//QStringList VMService::extractors();
bool VMService::notificationsEnabled() {
    return true;
}

void VMService::startTransfer(int transferId) {

}

int VMService::submitRequest(const QString &displayName, const QString &applicationIcon, const QString &serviceIcon, const QString &filePath, const QString &desiredResolution, const QStringList &callback, const QString &cancelMethod, const QString &restartMethod) {
    return 0;
}
