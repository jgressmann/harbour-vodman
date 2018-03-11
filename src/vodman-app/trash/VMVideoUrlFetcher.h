/*-
 * Copyright (c) 2015 Peter Tworek, 2017 Jean Gressmann
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <QCache>
//#include <QObject>
#include <QProcess>
#include <QVariantMap>
//#include <QJsonDocument>
#include <QMutex>



class VMVideoUrlFetcher: public QObject
{
    Q_OBJECT
public:
    ~VMVideoUrlFetcher();
    VMVideoUrlFetcher(QObject* parent = Q_NULLPTR);

    static void runInitialCheck();
    static bool available() { return _works; }
    static QString version() { return _version_str; }

    void fetchMetaData(int id, QUrl videoId);
    void fetchVod(int id, QUrl url, QString format, QString filePath);
    void cancelFetchVod(int id);

signals:
    void metaDataFetchComplete(int id, QVariantMap result);
    void vodStatusChanged(int id, QVariantMap result);
    void vodFetchCompleted(int id, QVariantMap result);

private slots:
    void onMetaDataProcessFinished(int, QProcess::ExitStatus);
    void onProcessError(QProcess::ProcessError);
    void onVodProcessStarted();
    void onVodProcessReadReady();

private:
    Q_DISABLE_COPY(VMVideoUrlFetcher)
    void cleanupProcess(QProcess* process);
    void dumpConnections();
    QVariantMap parseResponse(QJsonDocument doc);
private:
    QMutex m_Lock;
    QCache<QUrl, QVariantMap> _response_cache;
    QHash<QProcess*, QVariantMap> _processDataMap;
    QHash<int, QProcess*> _vodDownloads;
    QHash<QProcess*, QList<QMetaObject::Connection> > _connections;

private: // statics
    static QString _version_str;
    static bool _works;
};


