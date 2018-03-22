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

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QDataStream>
#include <QCommandLineOption>
#include <QScopedPointer>

#include "VMVodFileDownload.h"
#include "VMVodMetaDataDownload.h"
#include "service_interface.h" // http://inz.fi/2011/02/18/qmake-and-d-bus/

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

    QCommandLineOption verboseOption("verbose", "Verbose mode. Prints out more information.");
    QCommandLineOption outputOption(QStringList() << "o" << "output", "Write generated data into <file>.", "file");
    QCommandLineOption bestOption("best", "Downloads VOD in highest quality format.");
    QCommandLineOption worstOption("worst", "Downloads VOD in lowest quality format.");

    // works
//    QUrl url1(QStringLiteral("https://www.youtube.com/watch?v=7t-l0q_v4D8"));
//    QVariant v;
//    v.setValue(url1);
//    QUrl url2 = qvariant_cast<QUrl>(v);
//    qDebug() << url1 << url2;

    QDBusConnection connection = QDBusConnection::sessionBus();
    QScopedPointer<org::duckdns::jgressmann::vodman::service> vodman;
    vodman.reset(new org::duckdns::jgressmann::vodman::service("org.duckdns.jgressmann.vodman.service", "/instance", connection));

    QObject::connect(vodman.data(), &org::duckdns::jgressmann::vodman::service::vodMetaDataDownloadCompleted, &vodMetaDataDownloadCompleted);


    qDebug() << "Request meta data for" << "https://www.youtube.com/watch?v=7t-l0q_v4D8";
    auto pendingReply = vodman->newToken();
    pendingReply.waitForFinished();
    auto token = pendingReply.value();
    vodman->startFetchVodMetaData(token, QStringLiteral("https://www.youtube.com/watch?v=7t-l0q_v4D8"));

    return app.exec();
}
