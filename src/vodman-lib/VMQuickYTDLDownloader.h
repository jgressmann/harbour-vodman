/* The MIT License (MIT)
 *
 * Copyright (c) 2019 Jean Gressmann <jean@0x42.de>
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
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>

class QNetworkReply;

class VMQuickYTDLDownloader : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(QString ytdlVersion READ ytdlVersion NOTIFY ytdlVersionChanged)
    Q_PROPERTY(QString ytdlPath READ ytdlPath NOTIFY ytdlPathChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY isOnlineChanged)

public:
    enum Status
    {
        StatusReady,
        StatusUnavailable,
        StatusDownloading,
        StatusError,
    };
    Q_ENUMS(Status)

    enum Mode
    {
        ModeRelease,
        ModeTest
    };
    Q_ENUMS(Mode)

    enum Error
    {
        ErrorNone,
        ErrorPython,
        ErrorMalformedConfigurationFile,
        ErrorUnsupportedConfigurationFileFormat,
        ErrorNoSpaceLeftOnDevice,
        ErrorFileOrDirCreationFailed,
        ErrorDownloadFailed,
        ErrorOffline,
        ErrorBusy,
        ErrorUnknown,
    };
    Q_ENUMS(Error)

public:
    explicit VMQuickYTDLDownloader(QObject* parent = Q_NULLPTR);

public:
    QString ytdlVersion() const { return m_ytdlVersion; }
    QString ytdlPath() const { return m_ytdlPath; }
    Status status() const { return m_status; }
    void setStatus(Status value);
    Error error() const { return m_error; }
    void setError(Error value);
    Mode mode() const { return m_mode; }
    void setMode(Mode value);
    bool busy() const;
    bool isOnline() const { return m_networkConfigurationManager.isOnline(); }
    Q_INVOKABLE void download();
    Q_INVOKABLE void remove();

signals:
    void ytdlPathChanged();
    void ytdlVersionChanged();
    void statusChanged();
    void errorChanged();
    void modeChanged();
    void busyChanged();
    void isOnlineChanged();

private slots:
    void requestFinished(QNetworkReply *reply);
    void onlineStateChanged(bool);

private:
    enum DownloadStage
    {
        None,
        ConfigFile,
        Binary
    };

private:
    static bool findExecutable(const QString& name, const QString& path);
    void findBinary();
    Q_DISABLE_COPY(VMQuickYTDLDownloader)
    QString baseDir() const;
    QString modeDir(Mode value) const;

private:
    QNetworkConfigurationManager m_networkConfigurationManager;
    QNetworkAccessManager m_networkAccessManager;
    QString m_ytdlVersion;
    QString m_ytdlPath;
    QString m_configFilePath;
    Status m_status;
    Error m_error;
    int m_pythonVersion;
    DownloadStage m_stage;
    Mode m_mode;
    bool m_dead;
};
