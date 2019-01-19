/* The MIT License (MIT)
 *
 * Copyright (c) 2018, 2019 Jean Gressmann <jean@0x42.de>
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
    void startFetchVodMetaData(qlonglong token, const QString& url);
    void startFetchVodFile(qlonglong token, const QByteArray& vmVodFileDownloadRequest);
    void cancelFetchVodFile(qlonglong handle, bool deleteFile);
    QVariantList inProgressVodFileFetches();

signals:
    void vodMetaDataDownloadCompleted(qint64 handle, const QByteArray &result);
    void vodFileDownloadAdded(qlonglong handle, const QByteArray &result);
    void vodFileDownloadChanged(qlonglong handle, const QByteArray &result);
    void vodFileDownloadRemoved(qlonglong handle, const QByteArray &result);

private slots:
    void onFetchVodMetaDataCompleted(qint64, const VMVodMetaDataDownload &result);
    void onFetchVodFileCompleted(qint64, const VMVodFileDownload& result);
    void onFetchVodFileStatusChanged(qint64, const VMVodFileDownload& result);

private:
    QAtomicInteger<qint64> m_TokenGenerator;
    VMYTDL m_YoutubeDownloader;
};

