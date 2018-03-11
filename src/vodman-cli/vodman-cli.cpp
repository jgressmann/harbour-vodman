#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QDataStream>

#include "vodman_interface.h"
#include "VMVodFileDownload.h"
#include "VMVodMetaDataDownload.h"

static void vodMetaDataDownloadCompleted(qint64 handle, const QByteArray &result) {
    QDataStream s(result);
    VMVodMetaDataDownload download;
    s >>download;
    qDebug() << "handle" << handle << "vod meta data download" << download;
//    qApp->exit();
}


int
main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    // works
//    QUrl url1(QStringLiteral("https://www.youtube.com/watch?v=7t-l0q_v4D8"));
//    QVariant v;
//    v.setValue(url1);
//    QUrl url2 = qvariant_cast<QUrl>(v);
//    qDebug() << url1 << url2;

    QDBusConnection connection = QDBusConnection::sessionBus();
    auto vodman = new org::duckdns::jgressmann::vodman("org.duckdns.jgressmann.vodman", "/instance", connection);

    QObject::connect(vodman, &org::duckdns::jgressmann::vodman::vodMetaDataDownloadCompleted, &vodMetaDataDownloadCompleted);


    qDebug() << "Request meta data for" << "https://www.youtube.com/watch?v=7t-l0q_v4D8";
    auto pendingReply = vodman->newToken();
    pendingReply.waitForFinished();
    auto token = pendingReply.value();
    vodman->startFetchVodMetaData(token, QStringLiteral("https://www.youtube.com/watch?v=7t-l0q_v4D8"));

    return app.exec();
}
