/* The MIT License (MIT)
 *
 * Copyright (c) 2019-2020 Jean Gressmann <jean@0x42.de>
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
    Q_PROPERTY(Status downloadStatus READ downloadStatus NOTIFY downloadStatusChanged)
    Q_PROPERTY(Status updateStatus READ updateStatus NOTIFY updateStatusChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool isOnline READ isOnline NOTIFY isOnlineChanged)
    Q_PROPERTY(bool isUpdateAvailable READ isUpdateAvailable NOTIFY isUpdateAvailableChanged)
    Q_PROPERTY(QString updateVersion READ updateVersion NOTIFY updateVersionChanged)

public:
    enum Status
    {
        StatusReady,
        StatusUnavailable,
        StatusDownloading
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
        ErrorUnknown = -1,
        ErrorNone,
        ErrorPython,
        ErrorMalformedConfigurationFile,
        ErrorUnsupportedConfigurationFileFormat,
        ErrorNoSpaceLeftOnDevice,
        ErrorFileOrDirCreationFailed,
        ErrorDownloadFailed,
        ErrorOffline,
        ErrorBusy,
        ErrorTemporaryFailureNameResolution,
    };
    Q_ENUMS(Error)

public:
    explicit VMQuickYTDLDownloader(QObject* parent = Q_NULLPTR);

public:
    QString ytdlVersion() const { return m_ytdlVersion; }
    QString ytdlPath() const { return m_ytdlPath; }
    Status downloadStatus() const { return m_DownloadStatus; }
    Status updateStatus() const { return m_UpdateStatus; }
    Error error() const { return m_error; }
    Mode mode() const { return m_mode; }
    void setMode(Mode value);
    bool busy() const;
    bool isOnline() const { return m_networkConfigurationManager.isOnline(); }
    QString updateVersion() const { return m_UpdateVersion; }
    bool isUpdateAvailable() const;
    Q_INVOKABLE void download();
    Q_INVOKABLE void remove();
    Q_INVOKABLE void checkForUpdate();

signals:
    void ytdlPathChanged();
    void ytdlVersionChanged();
    void downloadStatusChanged();
    void errorChanged();
    void modeChanged();
    void busyChanged();
    void isOnlineChanged();
    void isUpdateAvailableChanged();
    void updateVersionChanged();
    void updateStatusChanged();

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
    void setError(Error value);
    void setDownloadStatus(Status value);
    void setUpdateStatus(Status value);

private:
    QNetworkConfigurationManager m_networkConfigurationManager;
    QNetworkAccessManager m_networkAccessManager;
    QString m_ytdlVersion;
    QString m_ytdlPath;
    QString m_configFilePath;
    QString m_UpdateVersion;
    Status m_DownloadStatus;
    Status m_UpdateStatus;
    Error m_error;
    int m_pythonVersion;
    DownloadStage m_stage;
    Mode m_mode;
    bool m_dead;
};
